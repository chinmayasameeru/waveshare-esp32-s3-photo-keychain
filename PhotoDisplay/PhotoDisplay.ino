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

static constexpr uint8_t BL_PIN = 1;
static constexpr uint8_t USER_BOOT_PIN = 0;
static constexpr uint8_t DEFAULT_BRIGHTNESS = 255;
static constexpr uint8_t MIN_BRIGHTNESS = 20;
static constexpr uint8_t DEFAULT_SLEEP_SECONDS = 30;

static constexpr uint32_t PHOTO_ADDR = 0x310000UL;
static constexpr uint32_t PHOTO_MAGIC_V1 = 0x544F4850UL; // "PHOT"
static constexpr uint32_t PHOTO_MAGIC_V2 = 0x324F4850UL; // "PHO2"
static constexpr uint16_t PHOTO_VERSION = 2;
static constexpr uint16_t PHOTO_HEADER_BYTES = 32;

struct __attribute__((packed)) PhotoHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t headerBytes;
  uint16_t width;
  uint16_t height;
  uint32_t jpegSize;
  uint32_t crc32;
  uint8_t brightness;
  uint8_t sleepSeconds;
  uint16_t flags;
  uint32_t reserved0;
  uint32_t reserved1;
};

static_assert(sizeof(PhotoHeader) == PHOTO_HEADER_BYTES, "PhotoHeader size mismatch");

uint8_t photoBrightness = DEFAULT_BRIGHTNESS;
uint8_t photoSleepSeconds = DEFAULT_SLEEP_SECONDS;
bool photoReady = false;
uint32_t lastActivityMs = 0;
int lastBootState = HIGH;

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

uint32_t crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFUL;

  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
  }

  return ~crc;
}

bool readPhotoHeader(PhotoHeader& header) {
  return readFlash(&header, PHOTO_ADDR, PHOTO_HEADER_BYTES);
}

bool showPhoto() {
  PhotoHeader header{};

  if (!readPhotoHeader(header)) {
    photoReady = false;
    messageScreen("PHOTO READ ERROR", "flash read failed", nullptr, TFT_RED);
    return false;
  }

  uint32_t jpegAddress = PHOTO_ADDR + PHOTO_HEADER_BYTES;
  uint32_t jpegSize = 0;

  if (header.magic == PHOTO_MAGIC_V2 &&
      header.version == PHOTO_VERSION &&
      header.headerBytes == PHOTO_HEADER_BYTES) {

    if (header.width != 320 || header.height != 240) {
      photoReady = false;
      messageScreen("PHOTO FORMAT", "image must be 320x240", nullptr, TFT_RED);
      return false;
    }

    if (header.brightness < MIN_BRIGHTNESS || header.brightness > DEFAULT_BRIGHTNESS) {
      photoReady = false;
      messageScreen("PHOTO SETTINGS", "invalid brightness", nullptr, TFT_RED);
      return false;
    }

    photoBrightness = header.brightness;
    photoSleepSeconds = header.sleepSeconds;
    jpegSize = header.jpegSize;

  } else if (header.magic == PHOTO_MAGIC_V1) {
    // Backward compatibility with the original 8-byte container.
    jpegAddress = PHOTO_ADDR + 8;
    jpegSize = (uint32_t)header.version |
               ((uint32_t)header.headerBytes << 16);
    photoBrightness = DEFAULT_BRIGHTNESS;
    photoSleepSeconds = DEFAULT_SLEEP_SECONDS;

  } else {
    photoReady = false;
    messageScreen("NO PHOTO", "choose an image in the flasher", "then flash again");
    return false;
  }

  const uint32_t flashSize = ESP.getFlashChipSize();

  if (jpegSize == 0 ||
      jpegAddress < PHOTO_ADDR ||
      jpegAddress + jpegSize > flashSize) {
    photoReady = false;
    messageScreen("PHOTO SIZE ERROR", "image does not fit flash", nullptr, TFT_RED);
    return false;
  }

  uint8_t* jpeg = (uint8_t*)ps_malloc(jpegSize);
  if (!jpeg) {
    photoReady = false;
    messageScreen("PHOTO MEMORY ERROR", "not enough PSRAM", nullptr, TFT_RED);
    return false;
  }

  if (!readFlash(jpeg, jpegAddress, jpegSize)) {
    free(jpeg);
    photoReady = false;
    messageScreen("PHOTO READ ERROR", "could not read image", nullptr, TFT_RED);
    return false;
  }

  if (header.magic == PHOTO_MAGIC_V2) {
    const uint32_t actualCrc = crc32(jpeg, jpegSize);
    if (actualCrc != header.crc32) {
      free(jpeg);
      photoReady = false;
      messageScreen("PHOTO VERIFY ERROR", "checksum failed", "please reflash photo", TFT_RED);
      return false;
    }
  }

  if (jpegSize < 4 ||
      jpeg[0] != 0xFF || jpeg[1] != 0xD8 ||
      jpeg[jpegSize - 2] != 0xFF || jpeg[jpegSize - 1] != 0xD9) {
    free(jpeg);
    photoReady = false;
    messageScreen("JPEG ERROR", "invalid JPEG data", "please reflash photo", TFT_RED);
    return false;
  }

  lcd.fillScreen(TFT_BLACK);

  const bool ok = lcd.drawJpg(
    jpeg, jpegSize,
    0, 0,
    lcd.width(), lcd.height(),
    0, 0,
    1.0f, 0.0f
  );

  free(jpeg);

  if (!ok) {
    photoReady = false;
    messageScreen("JPEG ERROR", "image could not be decoded", nullptr, TFT_RED);
    return false;
  }

  photoReady = true;
  lcd.setBrightness(photoBrightness);
  return true;
}

void enterLightSleep() {
  gpio_wakeup_enable((gpio_num_t)USER_BOOT_PIN, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  lcd.setBrightness(0);
  digitalWrite(BL_PIN, LOW);

  Serial.println("[SLEEP] light sleep; press BOOT to wake");
  esp_light_sleep_start();

  digitalWrite(BL_PIN, HIGH);
  lastActivityMs = millis();

  if (!showPhoto()) {
    lcd.setBrightness(DEFAULT_BRIGHTNESS);
  }

  Serial.println("[SLEEP] woke");
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(300);

  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, LOW);
  pinMode(USER_BOOT_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(DEFAULT_BRIGHTNESS);
  digitalWrite(BL_PIN, HIGH);

  Serial.printf(
    "[BOOT] Photo Display 2.0 | Flash=%lu PSRAM=%lu LCD=%dx%d\n",
    (unsigned long)ESP.getFlashChipSize(),
    (unsigned long)ESP.getPsramSize(),
    lcd.width(),
    lcd.height()
  );

  photoReady = showPhoto();
  lastActivityMs = millis();
}

void loop() {
  const int bootState = digitalRead(USER_BOOT_PIN);

  if (bootState == LOW) {
    digitalWrite(BL_PIN, HIGH);
    lcd.setBrightness(photoBrightness);
    lastActivityMs = millis();
  }

  if (lastBootState == LOW && bootState == HIGH) {
    lastActivityMs = millis();
  }

  lastBootState = bootState;

  if (photoReady &&
      photoSleepSeconds > 0 &&
      bootState == HIGH &&
      (millis() - lastActivityMs >= (uint32_t)photoSleepSeconds * 1000UL)) {
    enterLightSleep();
  }

  delay(10);
}
