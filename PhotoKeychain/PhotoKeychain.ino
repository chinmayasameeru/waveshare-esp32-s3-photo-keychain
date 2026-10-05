#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "FFat.h"
#include <esp_sleep.h>
#include "driver/gpio.h"

// Photo Keychain: stable photo transfer + power management

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
static constexpr uint8_t KEY_BOOT_PIN = 0;

static constexpr char AP_SSID[] = "PhotoKeychain";
static constexpr char AP_PASSWORD[] = "photo1234";

static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GATEWAY(192, 168, 4, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);

bool apRunning = false;
bool lastBootState = HIGH;
uint32_t lastDebounceMs = 0;

static constexpr uint32_t AP_INACTIVITY_MS = 5UL * 60UL * 1000UL;
static constexpr uint32_t NORMAL_SLEEP_DELAY_MS = 30UL * 1000UL;

WebServer webServer(80);
File photoFile;
bool storageReady = false;
bool photoWriteOk = false;
size_t photoWriteBytes = 0;
bool uploadInProgress = false;
bool finishRequested = false;
uint32_t lastActivityMs = 0;
uint32_t normalModeSinceMs = 0;

void markActivity();

static const char STAGE4_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#080b12">
<title>Photo Keychain</title>
<style>
:root{color-scheme:dark;--bg:#080b12;--card:#151a23;--card2:#0e131b;--line:#293342;--text:#edf2f7;--muted:#aab5c5;--soft:#8f9aae;--accent:#8fe3a0}
*{box-sizing:border-box}
body{margin:0;min-height:100vh;padding:18px;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:radial-gradient(circle at top,#17202d 0,#080b12 55%);color:var(--text)}
main{width:min(620px,100%);margin:auto}
.header{padding:8px 4px}.eyebrow{font-size:12px;font-weight:800;letter-spacing:.13em;color:var(--accent)}
h1{margin:5px 0 7px;font-size:32px;letter-spacing:-.03em}.sub{margin:0;color:var(--muted);line-height:1.55}
.card{margin-top:14px;padding:18px;border:1px solid var(--line);border-radius:18px;background:rgba(21,26,35,.96);box-shadow:0 14px 35px rgba(0,0,0,.16)}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px}.badge{display:inline-flex;align-items:center;gap:7px;padding:7px 10px;border-radius:999px;background:#18311f;color:var(--accent);font-weight:800;font-size:12px}.dot{width:7px;height:7px;border-radius:50%;background:var(--accent)}
.step{display:flex;gap:12px;align-items:flex-start}.num{width:30px;height:30px;flex:0 0 30px;border-radius:10px;background:#202a38;display:grid;place-items:center;font-weight:800}
.step b{display:block;margin-bottom:3px}.step span{color:var(--soft);font-size:13px;line-height:1.5}.spacer{height:12px}
label.button,button{display:block;width:100%;border:0;border-radius:13px;padding:14px 16px;font:700 16px/1.1 inherit}
label.button{background:#f4f6f8;color:#111;text-align:center}button{margin-top:12px;background:#e9eef5;color:#101318}button.secondary{background:#1c2430;color:var(--text);border:1px solid var(--line)}button:disabled{opacity:.45}
input[type=file]{position:absolute;left:-9999px}
.previewWrap{margin-top:14px;padding:10px;border:1px solid var(--line);border-radius:14px;background:#05070a}
canvas{display:none;width:100%;aspect-ratio:4/3;border-radius:9px;background:#000}.fileInfo{display:none;margin-top:10px;color:var(--soft);font-size:12px;line-height:1.5}
#status{margin-top:12px;min-height:20px;white-space:pre-wrap;color:#b9c4d5;font:13px/1.5 ui-monospace,SFMono-Regular,Menlo,monospace}
.tip{color:var(--soft);font-size:12px;line-height:1.55;margin:10px 0 0}hr{border:0;border-top:1px solid var(--line);margin:16px 0}
</style>
</head>
<body>
<main>
<section class="header">
<div class="eyebrow">LOCAL • PRIVATE • DIRECT</div>
<h1>PHOTO KEYCHAIN</h1>
<p class="sub">Choose a photo on your iPhone and save it directly to your keychain.</p>
</section>

<section class="card">
<div class="row"><strong>iPhone connection</strong><span class="badge"><span class="dot"></span>Connected</span></div>
<p class="tip">This page is served by the ESP32. Your photo never needs to leave the local Wi-Fi link.</p>
</section>

<section class="card">
<div class="step"><div class="num">1</div><div><b>Choose your photo</b><span>Pick an image from Photos or Files.</span></div></div>
<div class="spacer"></div>
<label class="button" for="photo">Choose Photo</label>
<input id="photo" type="file" accept="image/*">
<div class="previewWrap"><canvas id="preview" width="320" height="240"></canvas></div>
<div id="fileInfo" class="fileInfo"></div>
</section>

<section class="card">
<div class="step"><div class="num">2</div><div><b>Save to keychain</b><span>Safari prepares a 320 × 240 JPEG locally, then sends it directly to the ESP32.</span></div></div>
<button id="save" disabled>Save Photo to Keychain</button>
<div id="status">Choose a photo to begin.</div>
</section>

<section class="card">
<p class="tip">Your saved photo is now displayed. Wi-Fi stays available for up to 5 minutes without activity.</p>
</section>
</main>

<script>
const input=document.getElementById('photo');
const canvas=document.getElementById('preview');
const saveButton=document.getElementById('save');
const status=document.getElementById('status');
const fileInfo=document.getElementById('fileInfo');
const ctx=canvas.getContext('2d',{alpha:false});
let preparedBlob=null;

function setStatus(text){status.textContent=text;}
function loadImage(file){
  return new Promise((resolve,reject)=>{
    const url=URL.createObjectURL(file),img=new Image();
    img.onload=()=>{URL.revokeObjectURL(url);resolve(img)};
    img.onerror=()=>{URL.revokeObjectURL(url);reject(new Error('Safari could not decode this image'))};
    img.src=url;
  });
}
async function preparePhoto(file){
  const img=await loadImage(file);
  const scale=Math.max(320/img.naturalWidth,240/img.naturalHeight);
  const w=img.naturalWidth*scale,h=img.naturalHeight*scale;
  ctx.fillStyle='#000';ctx.fillRect(0,0,320,240);ctx.drawImage(img,(320-w)/2,(240-h)/2,w,h);
  preparedBlob=await new Promise((resolve,reject)=>{
    canvas.toBlob(blob=>blob?resolve(blob):reject(new Error('JPEG conversion failed')),'image/jpeg',0.90);
  });
  if(preparedBlob.size>1500000) throw new Error('Prepared photo is too large. Please choose another image.');
  canvas.style.display='block';fileInfo.style.display='block';
  fileInfo.textContent='Prepared locally • 320 × 240 JPEG • '+preparedBlob.size.toLocaleString()+' bytes';
  saveButton.disabled=false;setStatus('Ready to save.');
}
input.addEventListener('change',async()=>{
  preparedBlob=null;canvas.style.display='none';fileInfo.style.display='none';saveButton.disabled=true;
  const file=input.files&&input.files[0];
  if(!file){setStatus('Choose a photo to begin.');return}
  setStatus('Preparing photo locally in Safari...');
  try{await preparePhoto(file)}catch(err){setStatus('Could not prepare photo: '+(err.message||err))}
});
saveButton.addEventListener('click',async()=>{
  if(!preparedBlob)return;
  saveButton.disabled=true;setStatus('Saving photo to the keychain...');
  try{
    const form=new FormData();form.append('photo',preparedBlob,'photo.jpg');
    const response=await fetch('/save',{method:'POST',body:form});
    const text=await response.text();
    if(!response.ok)throw new Error(text||('HTTP '+response.status));
    setStatus(text||'Photo saved successfully.');
  }catch(err){setStatus('Save failed: '+(err.message||err));saveButton.disabled=false}
});
</script>
</body>
</html>
)HTML";

bool beginStorage() {
  if (storageReady) return true;
  if (FFat.begin(false)) {
    storageReady = true;
    Serial.printf("[FFAT] Mounted, total=%lu used=%lu\n",
                  (unsigned long)FFat.totalBytes(),
                  (unsigned long)FFat.usedBytes());
    return true;
  }

  Serial.println("[FFAT] Mount failed; formatting once...");
  if (!FFat.begin(true)) {
    Serial.println("[FFAT] Format/mount failed");
    return false;
  }

  storageReady = true;
  Serial.printf("[FFAT] Formatted + mounted, total=%lu used=%lu\n",
                (unsigned long)FFat.totalBytes(),
                (unsigned long)FFat.usedBytes());
  return true;
}

bool showStoredPhoto() {
  if (!beginStorage()) {
    Serial.println("[PHOTO] Storage unavailable");
    return false;
  }

  if (!FFat.exists("/photo.jpg")) {
    Serial.println("[PHOTO] No stored photo");
    return false;
  }

  File f = FFat.open("/photo.jpg", FILE_READ);
  if (!f) {
    Serial.println("[PHOTO] Failed to open /photo.jpg");
    return false;
  }

  const size_t size = f.size();
  if (size == 0 || size > 1500000UL) {
    Serial.printf("[PHOTO] Invalid stored size: %lu\n", (unsigned long)size);
    f.close();
    return false;
  }

  uint8_t* jpeg = (uint8_t*)ps_malloc(size);
  if (!jpeg) {
    Serial.println("[PHOTO] PSRAM allocation failed");
    f.close();
    return false;
  }

  size_t totalRead = 0;
  while (totalRead < size) {
    const size_t got = f.read(jpeg + totalRead, size - totalRead);
    if (got == 0) break;
    totalRead += got;
  }
  f.close();

  if (totalRead != size) {
    Serial.printf("[PHOTO] Read incomplete: %lu/%lu\n",
                  (unsigned long)totalRead,
                  (unsigned long)size);
    free(jpeg);
    return false;
  }

  lcd.fillScreen(TFT_BLACK);
  const bool ok = lcd.drawJpg(jpeg, size, 0, 0, lcd.width(), lcd.height(), 0, 0, 1.0f, 0.0f);
  free(jpeg);

  if (!ok) {
    Serial.println("[PHOTO] JPEG decode failed");
    return false;
  }

  Serial.printf("[PHOTO] Displayed stored photo (%lu bytes)\n", (unsigned long)size);
  return true;
}

void handlePhotoUpload() {
  HTTPUpload& upload = webServer.upload();

  switch (upload.status) {
    case UPLOAD_FILE_START: {
      markActivity();
      uploadInProgress = true;
      photoWriteOk = false;
      photoWriteBytes = 0;

      if (!beginStorage()) {
        Serial.println("[PHOTO] Storage unavailable at upload start");
        return;
      }

      if (FFat.exists("/photo.tmp")) {
        FFat.remove("/photo.tmp");
      }

      photoFile = FFat.open("/photo.tmp", FILE_WRITE);
      if (!photoFile) {
        Serial.println("[PHOTO] Could not open temporary file");
        return;
      }

      Serial.printf("[PHOTO] Save started: %s\n", upload.filename.c_str());
      break;
    }

    case UPLOAD_FILE_WRITE: {
      markActivity();
      if (!photoFile) return;
      const size_t written = photoFile.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        Serial.printf("[PHOTO] Write error: %lu/%lu\n",
                      (unsigned long)written,
                      (unsigned long)upload.currentSize);
        photoWriteOk = false;
        return;
      }

      photoWriteBytes += written;
      if (photoWriteBytes > 1500000UL) {
        Serial.println("[PHOTO] Temporary photo exceeds limit");
        photoWriteOk = false;
        uploadInProgress = false;
        photoFile.close();
        FFat.remove("/photo.tmp");
      }
      break;
    }

    case UPLOAD_FILE_END: {
      if (!photoFile) return;
      photoFile.flush();
      photoFile.close();

      if (photoWriteBytes == 0 || photoWriteBytes > 1500000UL) {
        FFat.remove("/photo.tmp");
        Serial.println("[PHOTO] Invalid final photo size");
        return;
      }

      if (FFat.exists("/photo.jpg")) {
        FFat.remove("/photo.jpg");
      }

      if (!FFat.rename("/photo.tmp", "/photo.jpg")) {
        FFat.remove("/photo.tmp");
        Serial.println("[PHOTO] Rename failed");
        return;
      }

      photoWriteOk = true;
      uploadInProgress = false;
      Serial.printf("[PHOTO] Save complete: %lu bytes\n", (unsigned long)photoWriteBytes);
      break;
    }

    case UPLOAD_FILE_ABORTED:
      if (photoFile) photoFile.close();
      FFat.remove("/photo.tmp");
      photoWriteOk = false;
      uploadInProgress = false;
      Serial.println("[PHOTO] Save aborted");
      break;

    default:
      break;
  }
}

void markActivity() {
  lastActivityMs = millis();
}

void centerText(const char* text, int y, int size = 2, uint16_t color = TFT_WHITE) {
  lcd.setTextDatum(lgfx::textdatum_t::middle_center);
  lcd.setTextColor(color);
  lcd.setTextSize(size);
  lcd.drawString(text, lcd.width() / 2, y);
}

void drawReadyScreen() {
  lcd.fillScreen(TFT_BLACK);
  lcd.drawRoundRect(14, 14, lcd.width() - 28, lcd.height() - 28, 14, TFT_DARKGREY);

  centerText("PHOTO KEYCHAIN", 44, 2, TFT_WHITE);
  centerText("READY", 82, 2, TFT_GREEN);
  centerText("Wi-Fi  OFF", 116, 2, TFT_LIGHTGREY);
  centerText("PRESS BOOT", 156, 2, TFT_WHITE);
  centerText("to choose a photo", 184, 1, TFT_LIGHTGREY);
  centerText("from your iPhone", 202, 1, TFT_LIGHTGREY);
}

void drawApScreen() {
  lcd.fillScreen(TFT_BLACK);
  lcd.drawRoundRect(10, 10, lcd.width() - 20, lcd.height() - 20, 14, TFT_DARKGREY);

  centerText("PHOTO KEYCHAIN", 34, 2, TFT_WHITE);
  centerText("IPHONE MODE", 64, 2, TFT_GREEN);
  centerText("Wi-Fi  ON", 92, 1, TFT_LIGHTGREY);
  centerText("PhotoKeychain", 120, 2, TFT_WHITE);
  centerText("PASS  photo1234", 146, 1, TFT_LIGHTGREY);
  centerText("192.168.4.1", 176, 2, TFT_WHITE);
  centerText("Auto-off  5 min", 210, 1, TFT_LIGHTGREY);
}

void drawPhotoSavedScreen() {
  lcd.fillScreen(TFT_BLACK);
  lcd.drawRoundRect(24, 34, lcd.width() - 48, 172, 16, TFT_DARKGREY);
  centerText("PHOTO SAVED", 82, 2, TFT_GREEN);
  centerText("✓", 122, 3, TFT_WHITE);
  centerText("Wi-Fi  OFF", 164, 2, TFT_LIGHTGREY);
}

void stopAccessPoint(bool prepareForSleep = true) {
  if (!apRunning) return;

  Serial.println("[WIFI] Shutting down access point...");
  webServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  apRunning = false;
  finishRequested = false;

  if (showStoredPhoto()) {
    drawPhotoSavedScreen();
    delay(450);
    showStoredPhoto();
  } else {
    drawReadyScreen();
  }

  if (prepareForSleep) {
    normalModeSinceMs = millis();
  }
}

void enterLightSleep() {
  if (apRunning || digitalRead(KEY_BOOT_PIN) == LOW) return;

  Serial.println("[SLEEP] Light sleep; GPIO0 will wake device");
  gpio_wakeup_enable((gpio_num_t)KEY_BOOT_PIN, GPIO_INTR_LOW_LEVEL);
  if (esp_sleep_enable_gpio_wakeup() != ESP_OK) {
    Serial.println("[SLEEP] Failed to enable GPIO wakeup");
    normalModeSinceMs = millis();
    return;
  }

  esp_light_sleep_start();
  Serial.println("[SLEEP] Woke from light sleep");
  normalModeSinceMs = millis();
}

void handleSaveComplete() {
  markActivity();

  if (!photoWriteOk) {
    webServer.send(500, "text/plain; charset=utf-8", "Photo could not be saved.");
    return;
  }

  webServer.send(200, "text/plain; charset=utf-8",
                 "Photo saved successfully. The LCD will show it.");

  delay(50);
  if (!showStoredPhoto()) {
    Serial.println("[PHOTO] Saved file could not be displayed");
  }
}

void handleRoot() {
  markActivity();
  webServer.send_P(200, "text/html; charset=utf-8", STAGE4_PAGE);
}

void handleStatus() {
  markActivity();
  const bool hasPhoto = storageReady && FFat.exists("/photo.jpg");
  String json = "{\"stage\":4,\"wifi\":\"on\",\"ip\":\"" +
                WiFi.softAPIP().toString() +
                "\",\"photo\":" + String(hasPhoto ? "true" : "false") + "}";
  webServer.send(200, "application/json", json);
}


void handleNotFound() {
  webServer.sendHeader("Location", "http://192.168.4.1/", true);
  webServer.send(302, "text/plain", "");
}

void startWebServer() {
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/status", HTTP_GET, handleStatus);
  webServer.on("/save", HTTP_POST, handleSaveComplete, handlePhotoUpload);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  Serial.println("[HTTP] Server started on port 80");
}

bool startAccessPoint() {
  if (!WiFi.mode(WIFI_AP)) {
    Serial.println("[WIFI] Failed to enter AP mode");
    return false;
  }

  if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET)) {
    Serial.println("[WIFI] Failed to configure AP IP");
    WiFi.mode(WIFI_OFF);
    return false;
  }

  if (!WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 1)) {
    Serial.println("[WIFI] Failed to start AP");
    WiFi.mode(WIFI_OFF);
    return false;
  }

  apRunning = true;
  finishRequested = false;
  lastActivityMs = millis();
  Serial.printf(
      "[WIFI] AP started: SSID=%s IP=%s\\n",
      AP_SSID,
      WiFi.softAPIP().toString().c_str());
  startWebServer();
  drawApScreen();
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(300);

  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, LOW);

  pinMode(KEY_BOOT_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);
  digitalWrite(BL_PIN, HIGH);

  // Normal startup: Wi-Fi stays off. A stored photo is shown; otherwise show setup UI.
  WiFi.mode(WIFI_OFF);
  if (!showStoredPhoto()) {
    drawReadyScreen();
  }
  normalModeSinceMs = millis();

  Serial.printf(
      "[BOOT] Photo Keychain | Flash=%lu PSRAM=%lu LCD=%dx%d\n",
      (unsigned long)ESP.getFlashChipSize(),
      (unsigned long)ESP.getPsramSize(),
      (int)lcd.width(),
      (int)lcd.height());
}

void loop() {
  if (apRunning) {
    webServer.handleClient();

    if (finishRequested && !uploadInProgress) {
      stopAccessPoint(true);
      return;
    }

    if (!uploadInProgress && (millis() - lastActivityMs) >= AP_INACTIVITY_MS) {
      Serial.println("[WIFI] 5-minute inactivity timeout");
      stopAccessPoint(true);
      return;
    }
  }

  const bool bootState = digitalRead(KEY_BOOT_PIN);

  if (bootState != lastBootState) {
    lastDebounceMs = millis();
    lastBootState = bootState;
  }

  if (!apRunning && bootState == LOW && (millis() - lastDebounceMs) > 40) {
    Serial.println("[BOOT] Starting iPhone access point...");
    startAccessPoint();

    // Wait for the physical button release so one press causes one transition.
    while (digitalRead(KEY_BOOT_PIN) == LOW) {
      delay(5);
    }
  }

  if (!apRunning && bootState == HIGH &&
      (millis() - normalModeSinceMs) >= NORMAL_SLEEP_DELAY_MS) {
    enterLightSleep();
  }

  delay(10);
}
