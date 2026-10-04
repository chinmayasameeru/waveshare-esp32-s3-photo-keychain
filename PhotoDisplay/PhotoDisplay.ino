#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <esp_flash.h>
#include <esp_sleep.h>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX() {
    auto bus = _bus.config();
    bus.spi_host = SPI3_HOST;
    bus.spi_mode = 0;
    bus.freq_write = 40000000;
    bus.freq_read = 16000000;
    bus.spi_3wire = false;
    bus.use_lock = true;
    bus.dma_channel = SPI_DMA_CH_AUTO;
    bus.pin_sclk = 39;
    bus.pin_mosi = 38;
    bus.pin_miso = 40;
    bus.pin_dc = 42;
    _bus.config(bus);
    _panel.setBus(&_bus);

    auto panel = _panel.config();
    panel.pin_cs = 45;
    panel.pin_rst = -1;
    panel.pin_busy = -1;
    panel.memory_width = 240;
    panel.memory_height = 320;
    panel.panel_width = 240;
    panel.panel_height = 320;
    panel.offset_x = 0;
    panel.offset_y = 0;
    panel.offset_rotation = 0;
    panel.readable = false;
    panel.invert = true;
    panel.rgb_order = false;
    panel.dlen_16bit = false;
    panel.bus_shared = true;
    _panel.config(panel);

    setPanel(&_panel);
  }
};

LGFX lcd;

bool isDimmed = false;
uint32_t lastActivityMs = 0;
int lastBootState = HIGH;

static constexpr uint8_t BL_PIN = 1;
static constexpr uint8_t USER_BOOT_PIN = 0;
static constexpr uint8_t FULL_BRIGHTNESS = 255;
static constexpr uint32_t SLEEP_AFTER_MS = 30UL * 1000UL;
static constexpr uint32_t PHOTO_MAGIC = 0x544F4850UL;
static constexpr uint32_t PHOTO_ADDR = 0x310000UL;
static constexpr uint32_t PHOTO_HEADER_BYTES = 8UL;

void centerText(const char* text, int y, int size = 2, uint16_t color = TFT_WHITE) {
  lcd.setTextDatum(lgfx::textdatum_t::middle_center);
  lcd.setTextColor(color);
  lcd.setTextSize(size);
  lcd.drawString(text, lcd.width() / 2, y);
}

void messageScreen(const char* a, const char* b = nullptr, const char* c = nullptr,
                   uint16_t color = TFT_WHITE) {
  lcd.fillScreen(TFT_BLACK);
  centerText(a, 75, 2, color);
  if (b) centerText(b, 115, 1, TFT_LIGHTGREY);
  if (c) centerText(c, 140, 1, TFT_LIGHTGREY);
}

bool readFlash(void* buffer, uint32_t address, uint32_t length) {
  return esp_flash_read(esp_flash_default_chip, buffer, address, length) == ESP_OK;
}

bool readU32LE(uint32_t address, uint32_t& value) {
  uint8_t b[4];
  if (!readFlash(b, address, 4)) return false;
  value = (uint32_t)b[0] |
          ((uint32_t)b[1] << 8) |
          ((uint32_t)b[2] << 16) |
          ((uint32_t)b[3] << 24);
  return true;
}

bool showPhoto() {
  uint32_t magic = 0;
  uint32_t photoSize = 0;

  if (!readU32LE(PHOTO_ADDR, magic) ||
      !readU32LE(PHOTO_ADDR + 4, photoSize)) {
    messageScreen("PHOTO READ ERROR", "flash read failed", nullptr, TFT_RED);
    return false;
  }

  const uint32_t flashSize = ESP.getFlashChipSize();
  if (magic != PHOTO_MAGIC ||
      photoSize == 0 ||
      PHOTO_ADDR + PHOTO_HEADER_BYTES + photoSize > flashSize) {
    messageScreen("NO PHOTO", "choose an image in the flasher", "then flash again");
    return false;
  }

  uint8_t* jpeg = (uint8_t*)ps_malloc(photoSize);
  if (!jpeg) {
    messageScreen("PHOTO ERROR", "not enough PSRAM", nullptr, TFT_RED);
    return false;
  }

  if (!readFlash(jpeg, PHOTO_ADDR + PHOTO_HEADER_BYTES, photoSize)) {
    free(jpeg);
    messageScreen("PHOTO READ ERROR", "could not read image", nullptr, TFT_RED);
    return false;
  }

  lcd.fillScreen(TFT_BLACK);
  const bool ok = lcd.drawJpg(
    jpeg, photoSize,
    0, 0,
    lcd.width(), lcd.height(),
    0, 0,
    1.0f, 0.0f
  );
  free(jpeg);

  if (!ok) {
    messageScreen("JPEG ERROR", "image could not be decoded", nullptr, TFT_RED);
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(300);

  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, LOW);

  lcd.init();
  lcd.setRotation(1);
  pinMode(USER_BOOT_PIN, INPUT_PULLUP);

  lcd.setBrightness(FULL_BRIGHTNESS);
  digitalWrite(BL_PIN, HIGH);
  lastActivityMs = millis();

  Serial.printf(
    "[BOOT] Photo Display | Flash=%lu PSRAM=%lu LCD=%dx%d\n",
    (unsigned long)ESP.getFlashChipSize(),
    (unsigned long)ESP.getPsramSize(),
    lcd.width(),
    lcd.height()
  );

  showPhoto();
}

void enterLightSleep() {
  // GPIO0 can wake ESP32-S3 from light-sleep without rebooting.
  // This is important because GPIO0 is also the USB boot/strapping button.
  gpio_wakeup_enable((gpio_num_t)USER_BOOT_PIN, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  lcd.setBrightness(0);
  digitalWrite(BL_PIN, LOW);
  isDimmed = true;

  Serial.println("[SLEEP] entering light-sleep; press BOOT to wake");
  esp_light_sleep_start();

  // Woken by the BOOT button. Light-sleep does not reset the application.
  digitalWrite(BL_PIN, HIGH);
  lcd.setBrightness(FULL_BRIGHTNESS);
  isDimmed = false;
  lastActivityMs = millis();

  Serial.println("[SLEEP] woke up");
}

void loop() {
  const int bootState = digitalRead(USER_BOOT_PIN);

  // BOOT button wakes the display immediately and keeps it bright while held.
  if (bootState == LOW) {
    lcd.setBrightness(FULL_BRIGHTNESS);
    digitalWrite(BL_PIN, HIGH);
    isDimmed = false;
    lastActivityMs = millis();
  }

  // Once the button is released, start the inactivity timer again.
  if (lastBootState == LOW && bootState == HIGH) {
    lastActivityMs = millis();
  }

  lastBootState = bootState;

  if (!isDimmed && bootState == HIGH &&
      (millis() - lastActivityMs >= SLEEP_AFTER_MS)) {
    enterLightSleep();
  }

  delay(10);
}
