#pragma once

#include <chrono>
#include <filesystem>
#include <string>

class config {
  std::filesystem::path m_conf_path{SW6106_DEFAULT_CONFIG_PATH};
  std::filesystem::path m_i2c_dev_path{};

  bool m_single_run = false;

  std::string m_gpio_chip;
  uint m_gpio_line;
  bool m_gpio_enabled = true;

  std::chrono::seconds m_poll_interval;

  int m_low_charge_voltage;
  int m_low_charge_percent;
  bool m_power_off_on_low_charge;

  void read_cli_args(int argc, const char **argv);
  void read_config_file();

public:
  config(int argc, const char **argv);

  std::filesystem::path get_conf_path() const;
  std::filesystem::path get_i2c_dev_path() const;

  bool get_single_run() const;

  std::string get_gpio_chip() const;
  bool get_gpio_enabled() const;
  uint get_gpio_line() const;

  std::chrono::seconds get_poll_interval() const;

  bool get_power_off_on_low_charge() const;
  uint get_low_charge_voltage() const;
  uint get_low_charge_percent() const;
};
