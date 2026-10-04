#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <esp_partition.h>

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

static constexpr uint8_t BL_PIN = 1;
static constexpr uint32_t PHOTO_MAGIC = 0x544F4850UL; // "PHOT" little-endian
static constexpr size_t PHOTO_HEADER_BYTES = 8;

void centerText(const char* text, int y, int size = 2, uint16_t color = TFT_WHITE) {
  lcd.setTextDatum(lgfx::textdatum_t::middle_center);
  lcd.setTextColor(color);
  lcd.setTextSize(size);
  lcd.drawString(text, lcd.width() / 2, y);
}

void messageScreen(const char* a,
                   const char* b = nullptr,
                   const char* c = nullptr,
                   uint16_t color = TFT_WHITE) {
  lcd.fillScreen(TFT_BLACK);
  centerText(a, 75, 2, color);
  if (b) centerText(b, 115, 1, TFT_LIGHTGREY);
  if (c) centerText(c, 140, 1, TFT_LIGHTGREY);
}

bool readU32(const esp_partition_t* part, size_t offset, uint32_t& value) {
  uint8_t b[4];
  if (esp_partition_read(part, offset, b, sizeof(b)) != ESP_OK) return false;
  value = (uint32_t)b[0] |
          ((uint32_t)b[1] << 8) |
          ((uint32_t)b[2] << 16) |
          ((uint32_t)b[3] << 24);
  return true;
}

bool showPhoto() {
  const esp_partition_t* part = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA,
    ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
    "photo"
  );

  if (!part) {
    messageScreen("PHOTO PARTITION", "not found", "flash the new build", TFT_RED);
    return false;
  }

  uint32_t magic = 0;
  uint32_t photoSize = 0;

  if (!readU32(part, 0, magic) || !readU32(part, 4, photoSize)) {
    messageScreen("PHOTO READ ERROR", "cannot read flash", nullptr, TFT_RED);
    return false;
  }

  if (magic != PHOTO_MAGIC ||
      photoSize == 0 ||
      photoSize > part->size - PHOTO_HEADER_BYTES) {
    messageScreen("NO PHOTO", "choose a JPEG in the flasher", "and flash again");
    return false;
  }

  uint8_t* jpeg = (uint8_t*)ps_malloc(photoSize);
  if (!jpeg) {
    messageScreen("PHOTO TOO LARGE", "not enough PSRAM", nullptr, TFT_RED);
    return false;
  }

  if (esp_partition_read(part, PHOTO_HEADER_BYTES, jpeg, photoSize) != ESP_OK) {
    free(jpeg);
    messageScreen("PHOTO READ ERROR", "flash read failed", nullptr, TFT_RED);
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
    messageScreen("JPEG ERROR", "photo could not be decoded", nullptr, TFT_RED);
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
  lcd.setBrightness(255);
  digitalWrite(BL_PIN, HIGH);

  Serial.printf(
    "[BOOT] Photo Display | Flash=%lu PSRAM=%lu LCD=%dx%d\n",
    (unsigned long)ESP.getFlashChipSize(),
    (unsigned long)ESP.getPsramSize(),
    lcd.width(),
    lcd.height()
  );

  showPhoto();
}

void loop() {
  delay(1000);
}
