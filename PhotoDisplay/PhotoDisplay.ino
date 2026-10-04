#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <FFat.h>
#include <FS.h>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX() {
    {
      auto c = _bus.config();
      c.spi_host = SPI3_HOST;
      c.spi_mode = 0;
      c.freq_write = 40000000;
      c.freq_read = 16000000;
      c.spi_3wire = false;
      c.use_lock = true;
      c.dma_channel = SPI_DMA_CH_AUTO;
      c.pin_sclk = 39;
      c.pin_mosi = 38;
      c.pin_miso = 40;
      c.pin_dc = 42;
      _bus.config(c);
      _panel.setBus(&_bus);
    }

    {
      auto c = _panel.config();
      c.pin_cs = 45;
      c.pin_rst = -1;
      c.pin_busy = -1;
      c.memory_width = 240;
      c.memory_height = 320;
      c.panel_width = 240;
      c.panel_height = 320;
      c.offset_x = 0;
      c.offset_y = 0;
      c.offset_rotation = 0;
      c.readable = false;
      c.invert = true;
      c.rgb_order = false;
      c.dlen_16bit = false;
      c.bus_shared = true;
      _panel.config(c);
    }

    setPanel(&_panel);
  }
};

LGFX lcd;
WebServer server(80);
DNSServer dnsServer;

static constexpr uint8_t BL_PIN = 1;
static constexpr uint8_t USER_BOOT_PIN = 0;
static constexpr uint16_t DNS_PORT = 53;

static const char* PHOTO_PATH = "/photo.jpg";
static const char* AP_SSID = "PhotoDisplay";
static const char* AP_PASSWORD = "photo1234";

static constexpr uint32_t AP_IDLE_MS = 60UL * 60UL * 1000UL;
static constexpr size_t MAX_PHOTO_BYTES = 4UL * 1024UL * 1024UL;

String apSsid;
bool apActive = false;
uint32_t lastClientAt = 0;

void textCenter(const char* text, int y, int size = 2, uint16_t color = TFT_WHITE) {
  lcd.setTextDatum(lgfx::textdatum_t::middle_center);
  lcd.setTextColor(color);
  lcd.setTextSize(size);
  lcd.drawString(text, lcd.width() / 2, y);
}

void drawNoPhotoScreen() {
  lcd.fillScreen(TFT_BLACK);

  const int h = lcd.height();
  for (int y = 0; y < h; ++y) {
    const uint8_t r = (uint8_t)(12 + (y * 80) / max(1, h - 1));
    const uint8_t g = (uint8_t)(18 + (y * 50) / max(1, h - 1));
    const uint8_t b = (uint8_t)(55 + ((h - 1 - y) * 120) / max(1, h - 1));
    lcd.drawFastHLine(0, y, lcd.width(), lcd.color565(r, g, b));
  }

  textCenter("PHOTO DISPLAY", 105, 2);
  textCenter("Connect your phone", 135, 1, TFT_LIGHTGREY);
  textCenter("to upload a JPEG", 152, 1, TFT_LIGHTGREY);
  textCenter("192.168.4.1", 184, 1, TFT_WHITE);
}

bool showStoredPhoto() {
  if (!FFat.exists(PHOTO_PATH)) {
    drawNoPhotoScreen();
    return false;
  }

  File f = FFat.open(PHOTO_PATH, FILE_READ);
  if (!f) {
    drawNoPhotoScreen();
    textCenter("Cannot open photo", lcd.height() - 20, 1, TFT_RED);
    return false;
  }

  const size_t photoSize = f.size();
  if (photoSize == 0 || photoSize > MAX_PHOTO_BYTES) {
    f.close();
    drawNoPhotoScreen();
    textCenter("Invalid photo file", lcd.height() - 20, 1, TFT_RED);
    return false;
  }

  // Copy the JPEG into PSRAM, then use LovyanGFX's raw-buffer JPEG API.
  uint8_t* jpeg = (uint8_t*)ps_malloc(photoSize);
  if (!jpeg) {
    f.close();
    drawNoPhotoScreen();
    textCenter("Not enough PSRAM", lcd.height() - 20, 1, TFT_RED);
    return false;
  }

  size_t got = 0;
  while (got < photoSize) {
    const size_t n = f.read(jpeg + got, photoSize - got);
    if (n == 0) break;
    got += n;
  }
  f.close();

  if (got != photoSize) {
    free(jpeg);
    drawNoPhotoScreen();
    textCenter("Photo read failed", lcd.height() - 20, 1, TFT_RED);
    return false;
  }

  lcd.fillScreen(TFT_BLACK);

  const bool ok = lcd.drawJpg(
    jpeg,
    photoSize,
    0,
    0,
    lcd.width(),
    lcd.height(),
    0,
    0,
    1.0f,
    0.0f
  );

  free(jpeg);

  if (!ok) {
    drawNoPhotoScreen();
    textCenter("JPEG decode failed", lcd.height() - 20, 1, TFT_RED);
    return false;
  }

  return true;
}
String pageHtml() {
  String s;
  s.reserve(9000);

  s += F("<!doctype html><html><head>");
  s += F("<meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'>");
  s += F("<meta name='apple-mobile-web-app-capable' content='yes'>");
  s += F("<title>Photo Display</title>");
  s += F("<style>");
  s += F(":root{color-scheme:dark}*{box-sizing:border-box}");
  s += F("body{margin:0;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0b0e14;color:#f2f5f8;padding:20px}");
  s += F(".wrap{max-width:620px;margin:0 auto}.card{background:#161b23;border:1px solid #2a3240;border-radius:18px;padding:20px;margin-bottom:16px}");
  s += F("h1{margin:0 0 7px;font-size:28px}p{color:#adb7c7}.small{font-size:14px;color:#8f9aae}");
  s += F("input{width:100%;font-size:17px;padding:13px;border-radius:12px;background:#0e131b;color:#fff;border:1px solid #394456}");
  s += F("button{width:100%;margin-top:12px;padding:14px;border:0;border-radius:12px;font-size:17px;font-weight:600;background:#f5f7fa;color:#111}");
  s += F("button.secondary{background:#272f3d;color:#f5f7fa}#status{margin-top:12px;min-height:22px}");
  s += F("</style></head><body><div class='wrap'>");

  s += F("<div class='card'><h1>Photo Display</h1>");
  s += F("<p>Waveshare ESP32-S3 2-inch · No Touch</p>");
  s += F("<p class='small'>Choose a JPEG. It is stored in Flash and shown on the LCD.</p></div>");

  s += F("<div class='card'><form id='uploadForm'>");
  s += F("<input id='photo' type='file' accept='image/jpeg,.jpg,.jpeg' required>");
  s += F("<button type='submit'>Show photo</button>");
  s += F("</form><div id='status' class='small'></div></div>");

  s += F("<div class='card'>");
  s += F("<button class='secondary' id='clear'>Clear photo</button>");
  s += F("<p class='small'>Wi-Fi turns off after 10 minutes with no browser activity.</p></div>");

  s += F("<script>");
  s += F("const f=document.getElementById('uploadForm'),p=document.getElementById('photo'),m=document.getElementById('status');");
  s += F("f.addEventListener('submit',async e=>{e.preventDefault();const file=p.files[0];if(!file)return;");
  s += F("if(file.size>8*1024*1024){m.textContent='Photo is larger than 8 MB.';return;}");
  s += F("m.textContent='Uploading...';const d=new FormData();d.append('photo',file,file.name);");
  s += F("try{const r=await fetch('/upload',{method:'POST',body:d});m.textContent=await r.text();}catch(err){m.textContent='Upload failed: '+err;}});");
  s += F("document.getElementById('clear').addEventListener('click',async()=>{m.textContent='Clearing...';const r=await fetch('/blank',{method:'POST'});m.textContent=await r.text();});");
  s += F("</script></div></body></html>");

  return s;
}

bool validJpegName(const String& name) {
  String n = name;
  n.toLowerCase();
  return n.endsWith(".jpg") || n.endsWith(".jpeg");
}

void drawApScreen(const char* status = nullptr) {
  // Rotation 1 makes the LCD 320x240. Keep all setup text inside 0..239.
  drawNoPhotoScreen();

  if (apSsid.length()) {
    textCenter("Wi-Fi:", 182, 1, TFT_WHITE);
    textCenter(apSsid.c_str(), 198, 1, TFT_WHITE);
    textCenter("Pass: photo1234", 214, 1, TFT_LIGHTGREY);
    textCenter("Open: 192.168.4.1", 232, 1, TFT_WHITE);
  }

  if (status) {
    textCenter(status, 220, 1, TFT_RED);
  }
}

void startAP() {
  if (apActive) {
    lastClientAt = millis();
    drawApScreen();
    return;
  }

  apSsid = AP_SSID;

  // Start the ESP32-S3 AP in the simplest possible configuration.
  // Keep it enabled for testing so the iPhone can always discover it.
  WiFi.mode(WIFI_OFF);
  delay(200);
  WiFi.mode(WIFI_AP);
  delay(500);
  WiFi.setSleep(false);

  IPAddress ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gateway, subnet);

  bool started = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 4);

  if (!started) {
    apActive = false;
    Serial.println("[AP] FAILED");
    drawApScreen("Wi-Fi AP FAILED");
    return;
  }

  const IPAddress actualIp = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", actualIp);

  apActive = true;
  lastClientAt = millis();

  Serial.printf("[AP] SSID=%s PASS=%s IP=%s\n",
                AP_SSID, AP_PASSWORD, actualIp.toString().c_str());

  if (FFat.exists(PHOTO_PATH)) {
    showStoredPhoto();
  } else {
    drawApScreen();
  }
}
void stopAP() {
  if (!apActive) return;

  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  apActive = false;

  Serial.println("[AP] stopped");
  showStoredPhoto();
}

void setupServer() {
  server.on("/", HTTP_GET, []() {
    lastClientAt = millis();
    server.send(200, "text/html; charset=utf-8", pageHtml());
  });

  server.on("/generate_204", HTTP_GET, []() {
    lastClientAt = millis();
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.on("/hotspot-detect.html", HTTP_GET, []() {
    lastClientAt = millis();
    server.send(200, "text/html; charset=utf-8", pageHtml());
  });

  server.on("/connecttest.txt", HTTP_GET, []() {
    lastClientAt = millis();
    server.send(200, "text/plain", "OK");
  });

  server.on("/ncsi.txt", HTTP_GET, []() {
    lastClientAt = millis();
    server.send(200, "text/plain", "Microsoft NCSI");
  });

  server.on("/upload", HTTP_POST,
    []() {
      lastClientAt = millis();
      server.send(200, "text/plain", "Photo uploaded and displayed.");
    },
    []() {
      HTTPUpload& up = server.upload();
      static File uploadFile;
      static bool accepted = false;
      static size_t totalWritten = 0;

      if (up.status == UPLOAD_FILE_START) {
        accepted = validJpegName(up.filename);
        totalWritten = 0;

        if (!accepted) {
          Serial.printf("[UPLOAD] rejected filename: %s\n", up.filename.c_str());
          return;
        }

        if (FFat.exists(PHOTO_PATH)) FFat.remove(PHOTO_PATH);

        uploadFile = FFat.open(PHOTO_PATH, FILE_WRITE);
        if (!uploadFile) {
          accepted = false;
          Serial.println("[UPLOAD] cannot open photo file");
          return;
        }

        Serial.printf("[UPLOAD] start: %s\n", up.filename.c_str());
      }
      else if (up.status == UPLOAD_FILE_WRITE) {
        if (!accepted || !uploadFile) return;

        if (totalWritten + up.currentSize > MAX_PHOTO_BYTES) {
          accepted = false;
          uploadFile.close();
          if (FFat.exists(PHOTO_PATH)) FFat.remove(PHOTO_PATH);
          Serial.println("[UPLOAD] rejected: file too large");
          return;
        }

        const size_t written = uploadFile.write(up.buf, up.currentSize);
        if (written != up.currentSize) {
          accepted = false;
          uploadFile.close();
          if (FFat.exists(PHOTO_PATH)) FFat.remove(PHOTO_PATH);
          Serial.println("[UPLOAD] write error");
          return;
        }

        totalWritten += written;
      }
      else if (up.status == UPLOAD_FILE_END) {
        if (uploadFile) uploadFile.close();

        if (!accepted || totalWritten == 0) {
          if (FFat.exists(PHOTO_PATH)) FFat.remove(PHOTO_PATH);
          Serial.println("[UPLOAD] failed");
          return;
        }

        Serial.printf("[UPLOAD] complete: %u bytes\n", (unsigned)totalWritten);
        showStoredPhoto();
      }
      else if (up.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile) uploadFile.close();
        if (FFat.exists(PHOTO_PATH)) FFat.remove(PHOTO_PATH);
        Serial.println("[UPLOAD] aborted");
      }
    }
  );

  server.on("/blank", HTTP_POST, []() {
    lastClientAt = millis();

    if (FFat.exists(PHOTO_PATH)) FFat.remove(PHOTO_PATH);

    drawNoPhotoScreen();
    server.send(200, "text/plain", "Photo cleared.");
  });

  server.on("/status", HTTP_GET, []() {
    lastClientAt = millis();

    String out = "{";
    out += "\"ap\":";
    out += apActive ? "true" : "false";
    out += ",\"photo\":";
    out += FFat.exists(PHOTO_PATH) ? "true" : "false";

    if (FFat.exists(PHOTO_PATH)) {
      File f = FFat.open(PHOTO_PATH, FILE_READ);
      out += ",\"bytes\":";
      out += f ? String((unsigned)f.size()) : "0";
      if (f) f.close();
    } else {
      out += ",\"bytes\":0";
    }

    out += "}";
    server.send(200, "application/json", out);
  });

  server.onNotFound([]() {
    lastClientAt = millis();
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(300);

  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, LOW);
  pinMode(USER_BOOT_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("[BOOT] Waveshare ESP32-S3 Photo Display");

  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);
  digitalWrite(BL_PIN, HIGH);

  Serial.printf(
    "[BOOT] Flash=%u bytes PSRAM=%u bytes LCD=%dx%d\n",
    (unsigned long)ESP.getFlashChipSize(),
    (unsigned long)ESP.getPsramSize(),
    (int)lcd.width(),
    (int)lcd.height()
  );

  if (!FFat.begin(true)) {
    Serial.println("[FFat] mount/format failed");
    lcd.fillScreen(TFT_RED);
    textCenter("FFat mount failed", lcd.height() / 2, 2, TFT_WHITE);
    delay(2000);
  } else {
    Serial.printf(
      "[FFat] total=%llu used=%llu\n",
      (unsigned long long)FFat.totalBytes(),
      (unsigned long long)FFat.usedBytes()
    );
  }

  drawNoPhotoScreen();
  setupServer();
  startAP();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  static int lastBoot = HIGH;
  const int boot = digitalRead(USER_BOOT_PIN);

  if (lastBoot == HIGH && boot == LOW) {
    startAP();
  }

  lastBoot = boot;

  if (apActive && (millis() - lastClientAt > AP_IDLE_MS)) {
    stopAP();
  }

  delay(2);
}
