// ============================================================================
//  Implementazione IO Extension Waveshare per ESPHome
//  Replica esattamente la sequenza del driver io_extension.c ufficiale.
// ============================================================================
#include "io_extension_ws.h"
#include "esphome/core/log.h"

namespace esphome {
namespace io_extension_ws {

static const char *const TAG = "io_extension_ws";

void IoExtensionWS::setup() {
  ESP_LOGCONFIG(TAG, "Setting up IO Extension Waveshare (0x24)...");

  // 1) MODE: tutti i pin in output
  if (!this->write_reg_(WS_REG_MODE, 0xFF)) {
    ESP_LOGE(TAG, "Errore scrittura MODE - chip non risponde su 0x24?");
    this->mark_failed();
    return;
  }

  // Riproduce esattamente IO_EXTENSION_Init() + wavesahre_rgb_lcd_bl_on()
  // dal sorgente Waveshare ufficiale (no PWM, solo OUTPUT=0xFF).
  this->output_state_ = 0xFF;
  bool ok2 = this->write_reg_(WS_REG_OUTPUT, this->output_state_);
  ESP_LOGI(TAG, "OUTPUT=0xFF: %s (state=0x%02X)", ok2 ? "OK" : "FAIL", this->output_state_);

  // Registro PWM (0x05): valore diretto -> 0 = minimo, valori alti = più
  // luminoso, 255 = spento (overflow). Il chip CONSERVA il valore tra i riavvii
  // dell'ESP32, quindi va impostato esplicitamente. Default luminoso ma sotto la
  // soglia di flicker (247) -> 200. Regolabile live via il number in HA.
  bool okp = this->write_reg_(WS_REG_PWM, 200);
  ESP_LOGI(TAG, "PWM=200 (luminoso): %s", okp ? "OK" : "FAIL");

  // Rifà tra 300ms — nel caso qualcosa lo resetti durante il boot.
  this->set_timeout(300, [this]() {
    bool ok = this->write_reg_(WS_REG_OUTPUT, this->output_state_);
    ESP_LOGI(TAG, "OUTPUT retry 300ms: %s (state=0x%02X)", ok ? "OK" : "FAIL", this->output_state_);
    this->write_reg_(WS_REG_PWM, 200);
  });

  ESP_LOGCONFIG(TAG, "IO Extension Waveshare pronto (sequenza Waveshare-identica).");
}

void IoExtensionWS::dump_config() {
  ESP_LOGCONFIG(TAG, "IO Extension Waveshare:");
  ESP_LOGCONFIG(TAG, "  Address: 0x24 (registro singolo)");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  COMUNICAZIONE FALLITA");
  }
}

bool IoExtensionWS::write_reg_(uint8_t reg, uint8_t value) {
  // Scrive 2 byte [reg][value] all'indirizzo I2C del device (0x24),
  // identico a DEV_I2C_Write_Nbyte -> i2c_master_transmit(addr, data, 2).
  uint8_t buf[2] = {reg, value};
  i2c::ErrorCode err = this->write(buf, 2);
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "write_reg_ fallita: reg=0x%02X val=0x%02X err=%d", reg, value, (int) err);
    return false;
  }
  return true;
}

void IoExtensionWS::set_output_pin(uint8_t pin, bool value) {
  if (pin > 7)
    return;
  if (value)
    this->output_state_ |= (1 << pin);
  else
    this->output_state_ &= ~(1 << pin);
  this->write_reg_(WS_REG_OUTPUT, this->output_state_);
  ESP_LOGD(TAG, "set_output_pin IO%d = %d (output_state=0x%02X)", pin, value, this->output_state_);
}

void IoExtensionWS::set_pwm(uint8_t value) {
  // Registro PWM (0x05) diretto -> 0 = minimo, valori alti = più luminoso,
  // 255 = spento (overflow del chip). Range utile pratico: 0..247.
  this->write_reg_(WS_REG_PWM, value);
  ESP_LOGD(TAG, "set_pwm = %d (0=min, 247=max)", value);
}

void IoExtensionWSGPIOPin::digital_write(bool value) {
  this->parent_->set_output_pin(this->pin_, value != this->inverted_);
}

std::string IoExtensionWSGPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "IO%u via io_extension_ws", this->pin_);
  return buffer;
}

}  // namespace io_extension_ws
}  // namespace esphome
