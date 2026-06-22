#pragma once
// ============================================================================
//  Componente custom ESPHome per l'IO Extension Waveshare (ESP32-S3-Touch-LCD-7B)
//  Replica il driver io_extension.c/.h ufficiale Waveshare.
//
//  Il chip e' a REGISTRO SINGOLO su indirizzo I2C 0x24:
//    ogni scrittura = [registro_interno][valore]  (2 byte)
//  Registri interni:
//    0x02 = MODE   (0xFF = tutti i pin in output)
//    0x03 = OUTPUT (stato pin 0-7)
//    0x04 = INPUT
//    0x05 = PWM    (luminosita' backlight, 0-255)
//  Assegnazione pin:
//    IO1 = reset touch (GT911)
//    IO2 = backlight ON/OFF
//    IO3 = reset LCD
//    IO4 = SD CS
// ============================================================================
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace io_extension_ws {

// Registri interni (dal datasheet/driver Waveshare)
static const uint8_t WS_REG_MODE = 0x02;
static const uint8_t WS_REG_OUTPUT = 0x03;
static const uint8_t WS_REG_INPUT = 0x04;
static const uint8_t WS_REG_PWM = 0x05;

class IoExtensionWS : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  // Imposta un pin di output (0-7) a livello alto/basso
  void set_output_pin(uint8_t pin, bool value);
  // Imposta la luminosita' backlight via PWM (0-255)
  void set_pwm(uint8_t value);

 protected:
  // scrive [reg][value] sul chip (2 byte all'indirizzo I2C)
  bool write_reg_(uint8_t reg, uint8_t value);
  // copia locale dello stato output (come Last_io_value nel driver Waveshare)
  uint8_t output_state_{0x00};
};

// GPIOPin che inoltra le scritture a un pin dell'expander.
// Permette di usare un pin dell'expander come reset_pin di altri componenti
// (es. il reset del touch GT911 su IO1).
class IoExtensionWSGPIOPin : public GPIOPin {
 public:
  void setup() override {}
  void pin_mode(gpio::Flags flags) override {}
  bool digital_read() override { return false; }
  void digital_write(bool value) override;
  std::string dump_summary() const override;
  gpio::Flags get_flags() const override { return this->flags_; }

  void set_parent(IoExtensionWS *parent) { this->parent_ = parent; }
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { this->flags_ = flags; }

 protected:
  IoExtensionWS *parent_;
  uint8_t pin_;
  bool inverted_;
  gpio::Flags flags_;
};

}  // namespace io_extension_ws
}  // namespace esphome
