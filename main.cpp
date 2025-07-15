#include "config.h"
#include "sw6106.h"

#include <chrono>
#include <csignal>
#include <ctime>
#include <gpiod.hpp>
#include <iostream>
#include <thread>

bool keep_running = false;

void sigint_handler(int signal, siginfo_t *, void *) {
  std::cout << "Caught signal " << signal;

  if (signal == SIGINT || signal == SIGQUIT || signal == SIGTERM) {
    std::cout << "; stopping...";
    keep_running = false;
  }

  std::cout << std::endl;
}

void register_signal_handlers() {
  struct sigaction sa;
  sa.sa_sigaction = sigint_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, const char **argv) {
  register_signal_handlers();
  config cfg(argc, argv);

  i2c::controller::ptr i2c_controller;

  i2c_controller = std::make_shared<i2c::controller>(cfg.get_i2c_dev_path());
  i2c_controller->open();

  keep_running = !cfg.get_single_run();
  const bool gpio_enabled = cfg.get_gpio_enabled();
  const std::string gpio_chip = cfg.get_gpio_chip();
  const std::chrono::seconds poll_interval = cfg.get_poll_interval();

  gpiod::chip gpio(cfg.get_gpio_chip());
  gpiod::line interrupt_line;

  if (keep_running && gpio_enabled) {
    interrupt_line = gpio.get_line(cfg.get_gpio_line());
    gpiod::line_request config;

    config.consumer = "Line";
    config.request_type = gpiod::line_request::EVENT_FALLING_EDGE;
    config.flags = gpiod::line_request::FLAG_BIAS_PULL_UP;
    interrupt_line.request(config);
  }

  sw6106 psu(i2c_controller);

  using irq = sw6106::interrupts;
  irq interrupts;
  interrupts = irq::ALL;

  psu.enable_interrupts(interrupts);
  interrupts = psu.read_interrupts(); // clear any pending interrupts
  auto status = psu.get_system_status();

  uint events = 0;
  uint charge_percent = 0;
  uint battery_voltage = 0;

  bool poweroff = false;
  bool charging = false;
  bool discharging = false;

  std::cout << "sw6106 chip version " << psu.get_chip_version() << std::endl;

  do {
    if (!keep_running || !gpio_enabled || events > 0 ||
        interrupt_line.get_value() == 0) {
      events = 0;

      charging =
          (static_cast<uint32_t>(status) &
           static_cast<uint32_t>(sw6106::system_status::CHARGER_CONNECTED));

      discharging = (static_cast<uint32_t>(status) &
                     static_cast<uint32_t>(
                         sw6106::system_status::BOOST_CONVERTER_ENABLED));

      charge_percent = psu.get_charge_percent();

      // I am well aware of std::chrono ability to print formatted time,
      // it's just bugged in the some versions of gcc.
      auto time = std::time(nullptr);
      auto localtime = std::localtime(&time);

      std::cout << "\n-----\n"
                << std::put_time(localtime, "%T") << "\nStatus:\n"
                << status << "\n\nCharge: " << charge_percent << '%';

      // Battery voltage will return an actual value only when something
      // actively working with a battery.
      if (charging || discharging) {
        battery_voltage = psu.get_battery_voltage_mv();

        std::cout << "\nBattery voltage: " << battery_voltage << " mV";
      }

      if (discharging)
        std::cout << "\nOutput voltage: " << psu.get_output_voltage_mv()
                  << " mV\nDischarge current: "
                  << psu.get_discharge_current_ma() << " mA";

      if (charging)
        std::cout << "\nCharge current: " << psu.get_charge_current_ma()
                  << " mA";

      if (!keep_running) {
        std::cout << std::endl;
        return 0;
      }

      if (interrupts != sw6106::interrupts::NONE)
        std::cout << "\nEvents:\n" << interrupts << std::endl;

      if (!charging && discharging && cfg.get_power_off_on_low_charge()) {
        if (charge_percent < cfg.get_low_charge_percent()) {
          std::cout << "\nCharge percent is bellow "
                    << cfg.get_low_charge_percent();
          poweroff = true;
        }

        if (battery_voltage < cfg.get_low_charge_voltage()) {
          std::cout << "\nBattery voltage is bellow "
                    << cfg.get_low_charge_voltage() << " mV";
          poweroff = true;
        }

        if (poweroff) {
          std::cout << ", powering off..." << std::endl;
          int res = system("poweroff");
          if (res == 0) {
            std::cout << "System accepted poweroff call, quitting..."
                      << std::endl;
            return 0;
          }

          std::cout << "\'poweroff\' system call failed!" << std::endl;
        }
      }
    }

    if (gpio_enabled) {
      try {
        if (interrupt_line.event_wait(std::chrono::seconds(5)))
          events = interrupt_line.event_read_multiple().size();
      } catch (std::system_error &) {
      }
    } else
      std::this_thread::sleep_for(poll_interval);

    interrupts = psu.read_interrupts();

    // Let the status registers to catch up
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    status = psu.get_system_status();

    // This will make journald happy
    std::cout << std::flush;
  } while (keep_running);

  return 0;
}
