#include "sw6106.h"

using bytes::byte;

/*
NOTICE! This device can only read/write one byte per transaction -_-
This means no fancy multi-byte reading and parsing in one go.
*/
static const byte sw6106_i2c_address = 0x3c;

static const byte interrupts_start = 0x05;
static const byte interrupts_end = 0x08;

static const byte global_interrupt_mask = 0x09;
static const byte interrupt_mask_start = 0x0a;
static const byte interrupt_mask_end = 0x0d;

static const byte system_status_register = 0x11;
static const byte adc_vbat_register = 0x14;
static const byte adc_vbat_vout_register = 0x15;
static const byte adc_vout_register = 0x16;

static const byte adc_ichg_register = 0x17;
static const byte adc_ichg_idischg_register = 0x18;
static const byte adc_idischg_register = 0x19;

static const byte chip_version_register = 0x26;
static const byte charge_percent_register = 0x4f;

template <class E>
std::string
bitflag_enum_to_string(const E val,
                       const std::map<E, std::string> &descriptions) {
  std::stringstream out;
  for (const auto &desc : descriptions) {
    // Very much type safe, as safe as it could possibly get.
    if (static_cast<std::underlying_type<E>::type>(val) &
        static_cast<std::underlying_type<E>::type>(desc.first)) {
      if (out.tellp() != std::streampos(0))
        out << '\n';
      out << '\t' << desc.second;
    }
  }

  return out.str();
}

const std::map<sw6106::system_status, std::string>
    sw6106::system_status_descriptions{
        {sw6106::system_status::PORT_A_CONNECTED,
         "USB type A port is connected"},
        {sw6106::system_status::PORT_MICRO_CONNECTED,
         "Micro USB port is connected"},
        {sw6106::system_status::PORT_C_CONNECTED,
         "USB type C port is connected"},
        {sw6106::system_status::CHARGER_CONNECTED, "Charger is connected"},
        {sw6106::system_status::BOOST_CONVERTER_ENABLED,
         "Boost converter is enabled"},
    };

const std::map<sw6106::interrupts, std::string> sw6106::interrupts_descriptions{
    {sw6106::interrupts::SHORT_CIRCUIT, "Short circuit protection triggered"},
    {sw6106::interrupts::IC_OVER_TEMPERATURE,
     "Integrated circuit overtemperature protection triggered"},
    {sw6106::interrupts::BATTERY_OVER_TEMPERATURE,
     "Battery overtemperature protection triggered"},
    {sw6106::interrupts::BATTERY_VOLTAGE_TOO_LOW, "Battery voltage is too low"},
    {sw6106::interrupts::CHARGE_TIMEOUT, "Battery charging is taking too long"},
    {sw6106::interrupts::MICRO_USB_OVERVOLTAGE,
     "Voltage on Micro USB port is too high"},
    {sw6106::interrupts::TYPE_C_OVERVOLTAGE,
     "Voltage on Type C port is too high"},
    {sw6106::interrupts::BATTERY_VOLTAGE_TOO_HIGH,
     "Battery voltage is too high"},
    {sw6106::interrupts::PORT_A_CONNECTED, "USB type A port is connected"},
    {sw6106::interrupts::PORT_A_DISCONNECTED,
     "USB type A port is disconnected"},
    {sw6106::interrupts::PORT_MICRO_CONNECTED, "Micro USB port is connected"},
    {sw6106::interrupts::PORT_MICRO_DISCONNECTED,
     "Micro USB port is disconnected"},
    {sw6106::interrupts::PORT_C_CONNECTED, "USB type C port is connected"},
    {sw6106::interrupts::PORT_C_DISCONNECTED,
     "USB type C port is disconnected"},
    {sw6106::interrupts::SHORT_CONTROL_KEY_PRESS,
     "Control key has been pressed shortly"},
    {sw6106::interrupts::FAST_CHARGE_STATUS_CHANGED,
     "Fast charge status changed"},
    {sw6106::interrupts::CHARGE_PERCENT_CHANGED, "Charge percent changed"},
    {sw6106::interrupts::BOOST_CONVERTER_DISABLED, "Boost converter disabled"},
    {sw6106::interrupts::BOOST_CONVERTER_ENABLED, "Boost converter enabled"},
    {sw6106::interrupts::CHARGER_DISABLED, "Charging disabled"},
    {sw6106::interrupts::CHARGER_ENABLED, "Charging enabled"},
    {sw6106::interrupts::CHARGE_BELLOW_5_PERCENT,
     "Charge level is bellow 5 percent"},
    {sw6106::interrupts::FULLY_CHARGED, "Battery is fully charged"},
    {sw6106::interrupts::WLED_STATE_CHANGED, "WLED state changed"},
};

sw6106::sw6106(i2c::controller::ptr controller)
    : i2c::peripheral(controller, sw6106_i2c_address) {}

void sw6106::enable_interrupts(const interrupts &i) {
  uint32_t interrupts = static_cast<uint32_t>(i);

  byte global_interrupts = 0;
  if (interrupts & static_cast<uint32_t>(interrupts::CATEGORY_0_INTERRUPTS))
    global_interrupts |= 1;

  if (interrupts & static_cast<uint32_t>(interrupts::CATEGORY_1_INTERRUPTS))
    global_interrupts |= (1 << 1);

  if (interrupts & static_cast<uint32_t>(interrupts::CATEGORY_2_INTERRUPTS))
    global_interrupts |= (1 << 2);

  if (interrupts & static_cast<uint32_t>(interrupts::CATEGORY_3_INTERRUPTS))
    global_interrupts |= (1 << 3);

  write(global_interrupt_mask, global_interrupts);

  for (byte mask_byte = interrupt_mask_start; mask_byte <= interrupt_mask_end;
       ++mask_byte) {

    byte mask_value = interrupts & 0xff;
    write(mask_byte, mask_value);
    interrupts >>= 8;
  }
}

sw6106::interrupts sw6106::read_interrupts() {
  uint32_t result = 0;
  for (byte b = interrupts_end; b >= interrupts_start; --b) {
    byte value = read(b);
    if (value)
      write(b, value);

    // suppress stupid type-safety warnings with those stupid casts
    result <<= 8;
    result |= value;
  }

  return static_cast<sw6106::interrupts>(result);
}

sw6106::system_status sw6106::get_system_status() {
  return system_status{
      static_cast<system_status>(read(system_status_register))};
}

unsigned int sw6106::get_chip_version() { return read(chip_version_register); }

unsigned sw6106::get_charge_percent() { return read(charge_percent_register); }

unsigned sw6106::get_battery_voltage_mv() {
  /*
  Rebuild voltage in millivolts according to formula provided in i2c
  register map: VBAT = ((Reg0x15[3:0]<<8) + Reg0x14[7:0]) * 1.2 mV
  */
  uint16_t voltage_mv = read(adc_vbat_vout_register);
  voltage_mv &= 0x0f;
  voltage_mv <<= 8;
  voltage_mv += read(adc_vbat_register);
  voltage_mv *= 1.2;

  return voltage_mv;
}

unsigned int sw6106::get_output_voltage_mv() {
  /*
  Rebuild voltage in millivolts according to formula provided in i2c
  register map: Vout = ((Reg0x15[7:4]<<8) + Reg0x16[7:0]) * 4 mV
  */
  uint16_t voltage_mv = read(adc_vbat_vout_register);
  voltage_mv &= 0xf0;
  voltage_mv <<= 4;
  voltage_mv += read(adc_vout_register);
  voltage_mv *= 4;

  return voltage_mv;
}

unsigned int sw6106::get_charge_current_ma() {
  /*
  Rebuild amperage in milliamps according to formula provided in i2c
  register map:  ICharge = ((Reg0x18 [3:0] << 8) + Reg0x17 [7:0]) * 25 / 7 mA
  */

  static const double ichg_conversion_coeff = 25. / 7.;

  uint16_t amps_ma = read(adc_ichg_idischg_register);
  amps_ma &= 0x0f;
  amps_ma <<= 8;
  amps_ma += read(adc_ichg_register);
  amps_ma *= ichg_conversion_coeff;

  return amps_ma;
}

unsigned int sw6106::get_discharge_current_ma() {
  /*
  Rebuild amperage in milliamps according to formula provided in i2c
  register map:  IDischarge = ((Reg0x18[7:4] << 8) + Reg0x19[7:0])* 25 / 7 mA
  */

  static const double idischg_conversion_coeff = 25. / 7.;

  uint16_t amps_ma = read(adc_ichg_idischg_register);
  amps_ma &= 0xf0;
  amps_ma <<= 4;
  amps_ma += read(adc_idischg_register);
  amps_ma *= idischg_conversion_coeff;

  return amps_ma;
}

std::ostream &operator<<(std::ostream &out, const sw6106::system_status &s) {
  if (s == sw6106::system_status::NONE)
    out << "\tIdle";
  else
    out << bitflag_enum_to_string(s, sw6106::system_status_descriptions);

  return out;
}

std::ostream &operator<<(std::ostream &out, const sw6106::interrupts &i) {
  if (i != sw6106::interrupts::NONE)
    out << bitflag_enum_to_string(i, sw6106::interrupts_descriptions);

  return out;
}
