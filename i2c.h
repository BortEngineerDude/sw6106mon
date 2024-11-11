// By gh/BortEngineerDude
#pragma once
#include "byte_util.h"
#include <filesystem>
#include <memory>

// A wrapper for a Linux I2C "adapter file".

namespace i2c {

using namespace bytes;
namespace fs = std::filesystem;

class peripheral;

class controller {
  friend class i2c::peripheral;

  fs::path m_file_path;
  int m_file_descriptor = -1;

  void write_vect(const byte devaddr, const vect &data);

  void write(const byte devaddr, const vect &reg, const vect &payload);
  void write(const byte devaddr, const vect &reg, const byte payload);
  void write(const byte devaddr, const byte reg, const vect &payload);
  void write(const byte devaddr, const byte reg, const byte payload);

  vect read(const byte devaddr, const vect &reg, const uint bytes);
  vect read(const byte devaddr, const byte reg, const uint bytes);
  byte read(const byte devaddr, const vect &reg);
  byte read(const byte devaddr, const byte reg);

public:
  using ptr = std::shared_ptr<i2c::controller>;
  controller(const fs::path &device);
  ~controller();

  void open();
  bool is_open();
  void close();
};

class peripheral {
protected:
  controller::ptr m_controller;
  byte m_address{0};

  peripheral(controller::ptr, const byte address);

  void write(const vect &reg, const vect &payload);
  void write(const vect &reg, const byte payload);
  void write(const byte reg, const vect &payload);
  void write(const byte reg, const byte payload);

  vect read(const vect &reg, const uint bytes);
  vect read(const byte reg, const uint bytes);
  byte read(const vect &reg);
  byte read(const byte reg);
};

} // namespace i2c
