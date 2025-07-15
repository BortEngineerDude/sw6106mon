#include "config.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define CONF_PARAM(X) static const std::string X = QUOTE(X);

CONF_PARAM(i2c_dev)
CONF_PARAM(gpio_interrupt_chip)
CONF_PARAM(gpio_interrupt_line)
CONF_PARAM(poll_interval)
CONF_PARAM(low_charge_voltage_mv)
CONF_PARAM(low_charge_percent)

void config::read_cli_args(int argc, const char **argv) {
  for (int argno = 1; argno < argc; ++argno) {
    std::string arg = argv[argno];

    if (arg == "-h" || arg == "--help") {
      std::cout
          << argv[0] << " options:\n"
          << "\t-h | --help :\t\tprint this help\n"
             "\t-s | --single-run :\tquery once and exit\n"
             "\t-i | --i2c_dev : \toverride i2c device (will ignore similar "
             "option in config file)\n"
             "\t-c | --config :\t\tset config path. Default value: "
          << SW6106_DEFAULT_CONFIG_PATH << std::endl;

      exit(0);
    }

    if (arg == "-s" || arg == "--single-run") {
      m_single_run = true;
      continue;
    }

    if (arg == "-i" || arg == "--i2c_dev") {
      if (argno + 1 >= argc)
        throw std::invalid_argument("I2C device argument missing");

      m_i2c_dev_path = argv[argno + 1];
      ++argno;
      continue;
    }

    if (arg == "-c" || arg == "--config") {
      if (argno + 1 >= argc)
        throw std::invalid_argument("Config argument missing");

      m_conf_path = argv[argno + 1];
      ++argno;
      continue;
    }

    throw std::invalid_argument("Unknown option: " + arg);
  }
}

void config::read_config_file() {
  std::ifstream cfg(m_conf_path);
  if (!cfg.is_open())
    throw std::invalid_argument(std::string("Failed to open config file ") +
                                m_conf_path.generic_string());

  std::string line;
  uint lineno = 0;

  std::set<std::string> options_to_find = {
      i2c_dev,       gpio_interrupt_chip,   gpio_interrupt_line,
      poll_interval, low_charge_voltage_mv, low_charge_percent};

  std::set<std::string> options_found;

  while (getline(cfg, line)) {
    ++lineno;
    if (line.size() == 0)
      continue;

    std::stringstream tokenize(line, std::ios_base::in);
    std::string option;
    tokenize >> option;

    if (option.starts_with('#'))
      continue;

    if (options_found.contains(option))
      throw std::invalid_argument("Redefenition of \"" + option +
                                  "\" at line " + std::to_string(lineno));

    if (options_to_find.contains(option)) {
      options_found.insert(option);
      options_to_find.erase(option);
    } else
      throw std::invalid_argument("Unknown option \"" + option + "\" at line " +
                                  std::to_string(lineno));

    std::string equals;
    tokenize >> equals;

    if (equals != "=")
      throw std::invalid_argument("Syntax error at line " +
                                  std::to_string(lineno));

    if (m_i2c_dev_path.empty() && option == i2c_dev)
      tokenize >> m_i2c_dev_path;

    if (option == gpio_interrupt_chip)
      tokenize >> m_gpio_chip;

    if (option == gpio_interrupt_line)
      tokenize >> m_gpio_line;

    if (option == poll_interval) {
      int arg;
      tokenize >> arg;
      if (arg <= 0)
        throw std::invalid_argument(
            "poll_interval should have a value greater than 0");

      m_poll_interval = std::chrono::seconds(arg);
    }

    if (option == low_charge_voltage_mv) {
      tokenize >> m_low_charge_voltage;
      if (m_low_charge_voltage < 2000 || m_low_charge_voltage > 5000)
        throw std::invalid_argument(
            "low_charge_voltage_mv should have a value between 2000 and 5000");
    }

    if (option == low_charge_percent) {
      tokenize >> m_low_charge_percent;
      if (m_low_charge_percent < 1 || m_low_charge_percent > 100)
        throw std::invalid_argument(
            "low_charge_percent should have a value between 1 and 100");
    }
  }

  if (cfg.bad())
    throw std::runtime_error("Error while reading config file " +
                             m_conf_path.generic_string());

  m_gpio_enabled = !options_to_find.contains(gpio_interrupt_chip) &&
                   !options_to_find.contains(gpio_interrupt_line);

  if (!m_single_run && !m_gpio_enabled &&
      options_to_find.contains(poll_interval))
    throw std::logic_error("Both poll_interval and GPIO interrupts are "
                           "disabled, unable to continue");

  m_power_off_on_low_charge =
      m_low_charge_percent > 0 || m_low_charge_voltage > 0;
}

config::config(int argc, const char **argv) {
  read_cli_args(argc, argv);

  if (m_single_run && !m_i2c_dev_path.empty())
    return;

  read_config_file();
}

std::filesystem::path config::get_conf_path() const { return m_conf_path; }

std::filesystem::path config::get_i2c_dev_path() const {
  return m_i2c_dev_path;
}

bool config::get_single_run() const { return m_single_run; }

std::string config::get_gpio_chip() const { return m_gpio_chip; }

bool config::get_gpio_enabled() const { return m_gpio_enabled; }

uint config::get_gpio_line() const { return m_gpio_line; }

std::chrono::seconds config::get_poll_interval() const {
  return m_poll_interval;
}

bool config::get_power_off_on_low_charge() const {
  return m_power_off_on_low_charge;
}

uint config::get_low_charge_voltage() const { return m_low_charge_voltage; }

uint config::get_low_charge_percent() const { return m_low_charge_percent; }
