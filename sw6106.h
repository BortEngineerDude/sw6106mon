#pragma once

#include "i2c.h"

#include <map>
#include <string>

class sw6106 : protected i2c::peripheral {
  using byte = bytes::byte;

public:
  sw6106(i2c::controller::ptr controller);

  /**
   * The system_status enum. Keep in mind, none of flags are mutually
   * exclusive, i.e system can be charging via the USB type C port and
   * discharging via USB type A port simultaniously!
   */
  enum class system_status : byte {
    NONE = 0,
    PORT_A_CONNECTED = 1,
    PORT_MICRO_CONNECTED = (1 << 1),
    PORT_C_CONNECTED = (1 << 2),
    CHARGER_CONNECTED = (1 << 4),
    BOOST_CONVERTER_ENABLED = (1 << 5)
  };

  static const std::map<sw6106::system_status, std::string>
      system_status_descriptions;

  enum class interrupts : uint32_t {
    NONE = 0,
    SHORT_CIRCUIT = 1,
    IC_OVER_TEMPERATURE = (1 << 1),
    // (1 << 2) skipped
    BATTERY_OVER_TEMPERATURE = (1 << 3),
    BATTERY_VOLTAGE_TOO_LOW = (1 << 4),
    CHARGE_TIMEOUT = (1 << 5),
    MICRO_USB_OVERVOLTAGE = (1 << 6),
    TYPE_C_OVERVOLTAGE = (1 << 7),
    BATTERY_VOLTAGE_TOO_HIGH = (1 << 8),
    PORT_A_CONNECTED = (1 << 9),
    PORT_A_DISCONNECTED = (1 << 10),
    PORT_MICRO_CONNECTED = (1 << 11),
    PORT_MICRO_DISCONNECTED = (1 << 12),
    PORT_C_CONNECTED = (1 << 13),
    PORT_C_DISCONNECTED = (1 << 14),
    SHORT_CONTROL_KEY_PRESS = (1 << 15),
    FAST_CHARGE_STATUS_CHANGED = (1 << 16),
    CHARGE_PERCENT_CHANGED = (1 << 17),
    BOOST_CONVERTER_ENABLED = (1 << 18),
    BOOST_CONVERTER_DISABLED = (1 << 19),
    CHARGER_ENABLED = (1 << 20),
    CHARGER_DISABLED = (1 << 21),
    CHARGE_BELLOW_5_PERCENT = (1 << 22),
    // (1 << 23) is skipped
    FULLY_CHARGED = (1 << 24),
    WLED_STATE_CHANGED = (1 << 25),

    CATEGORY_0_INTERRUPTS =
        SHORT_CIRCUIT | IC_OVER_TEMPERATURE | BATTERY_OVER_TEMPERATURE |
        BATTERY_OVER_TEMPERATURE | CHARGE_TIMEOUT | MICRO_USB_OVERVOLTAGE |
        TYPE_C_OVERVOLTAGE | BATTERY_VOLTAGE_TOO_HIGH,

    CATEGORY_1_INTERRUPTS = PORT_A_CONNECTED | PORT_A_DISCONNECTED |
                            PORT_MICRO_CONNECTED | PORT_MICRO_DISCONNECTED |
                            PORT_C_CONNECTED | PORT_C_DISCONNECTED |
                            SHORT_CONTROL_KEY_PRESS,

    CATEGORY_2_INTERRUPTS = FAST_CHARGE_STATUS_CHANGED |
                            BOOST_CONVERTER_ENABLED | BOOST_CONVERTER_DISABLED |
                            CHARGER_ENABLED | CHARGER_DISABLED,

    CATEGORY_3_INTERRUPTS = CHARGE_PERCENT_CHANGED | CHARGE_BELLOW_5_PERCENT |
                            WLED_STATE_CHANGED | FULLY_CHARGED,

    ALL = CATEGORY_0_INTERRUPTS | CATEGORY_1_INTERRUPTS |
          CATEGORY_2_INTERRUPTS | CATEGORY_3_INTERRUPTS

  };

  static const std::map<sw6106::interrupts, std::string>
      interrupts_descriptions;

  /**
   * Enable or disable interrupts. Set bits using interrupts::flags enum
   */
  void enable_interrupts(const interrupts &i);

  /**
   * Read and clear any pending interrupts.
   */
  interrupts read_interrupts();

  /**
   * Read system status. Refer to \ref system_status for more details.
   * @return system_status struct.
   */
  system_status get_system_status();

  /**
   * Read SW6106 chip version register.
   * @return Should always return 6.
   */
  unsigned get_chip_version();

  /**
   * Read compensated battery charge percent.
   * @return Value between 0-100 inclusive, representing current battery
   * charge.
   */
  unsigned get_charge_percent();

  /**
   * Read battery voltage.
   * @note Will read as 0 mV if system is in idle state.
   * @return Battery voltage in millivolts.
   */
  unsigned get_battery_voltage_mv();

  /**
   * Read device output voltage.
   * @return Device output voltage in millivolts.
   */
  unsigned get_output_voltage_mv();

  /**
   * @brief Read charge current.
   * @return Charge current in milliampers.
   */
  unsigned get_charge_current_ma();

  /**
   * @brief Read discharge current.
   * @return Discharge current in milliampers.
   */
  unsigned get_discharge_current_ma();
};

std::ostream &operator<<(std::ostream &out, const sw6106::system_status &s);
std::ostream &operator<<(std::ostream &out, const sw6106::interrupts &i);
