#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

#define FIRMWARE_VERSION "0.3.0"

MatrixPanel_I2S_DMA *display = nullptr;

// Farben
uint16_t black;
uint16_t white;
uint16_t green;
uint16_t blue;
uint16_t red;
uint16_t yellow;

// -----------------------------
// Display Modes
// -----------------------------
enum DisplayMode {
  MODE_SCROLL_TEXT = 0,
  MODE_LOGO_STATIC = 1,
  MODE_PIXEL_ART   = 2,
  MODE_RANDOM_FX   = 3
};

DisplayMode currentMode = MODE_SCROLL_TEXT;

// Demo-Automatik: schaltet alle paar Sekunden den Modus weiter.
// Später ersetzen wir das durch Webinterface-Auswahl.
bool autoModeDemo = true;
unsigned long lastModeChange = 0;
const unsigned long modeInterval = 10000;

// -----------------------------
// Laufschrift
// -----------------------------
const char *scrollText = "ELEKTRONIKSERVICE  -  REPARATUR  -  KONSOLEN  -  SMARTFIX  ";
int16_t scrollX = PANEL_RES_X;
unsigned long lastScrollUpdate = 0;
const unsigned long scrollInterval = 35;

// -----------------------------
// Pixel / FX
// -----------------------------
unsigned long lastFxUpdate = 0;
unsigned long lastFullRedraw = 0;

// -----------------------------
// Helper
// -----------------------------
int16_t getTextPixelWidth(const char *text) {
  return strlen(text) * 6;
}

const char *getModeName(DisplayMode mode) {
  switch (mode) {
    case MODE_SCROLL_TEXT: return "SCROLL_TEXT";
    case MODE_LOGO_STATIC: return "LOGO_STATIC";
    case MODE_PIXEL_ART:   return "PIXEL_ART";
    case MODE_RANDOM_FX:   return "RANDOM_FX";
    default:               return "UNKNOWN";
  }
}

void setMode(DisplayMode newMode) {
  currentMode = newMode;

  display->clearScreen();

  scrollX = PANEL_RES_X;
  lastScrollUpdate = 0;
  lastFxUpdate = 0;
  lastFullRedraw = 0;

  Serial.print("Mode changed to: ");
  Serial.println(getModeName(currentMode));
}

// -----------------------------
// Matrix Init
// -----------------------------
void initMatrix() {
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN
  );

  // Waveshare ESP32-S3-RGB-Matrix Settings
  mxconfig.gpio.e = 9;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

  display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!display->begin()) {
    Serial.println("ERROR: display->begin() failed!");
    while (true) {
      delay(1000);
    }
  }

  display->setBrightness8(70);
  display->clearScreen();

  black  = display->color565(0, 0, 0);
  white  = display->color565(255, 255, 255);
  green  = display->color565(0, 255, 80);
  blue   = display->color565(0, 120, 255);
  red    = display->color565(255, 0, 0);
  yellow = display->color565(255, 180, 0);
}

// -----------------------------
// Gemeinsame Zeichenelemente
// -----------------------------
void drawHeader() {
  display->setTextWrap(false);
  display->setTextSize(1);

  display->setCursor(2, 3);
  display->setTextColor(green);
  display->print("Smart");

  display->setCursor(34, 3);
  display->setTextColor(blue);
  display->print("Fix");

  display->drawLine(0, 13, 63, 13, blue);
}

// -----------------------------
// Mode 0: Laufschrift
// -----------------------------
void drawScrollingText() {
  unsigned long now = millis();

  if (now - lastScrollUpdate >= scrollInterval) {
    lastScrollUpdate = now;

    display->fillScreen(black);
    drawHeader();

    display->setTextWrap(false);
    display->setTextSize(1);
    display->setTextColor(white);
    display->setCursor(scrollX, 20);
    display->print(scrollText);

    scrollX--;

    int16_t textWidth = getTextPixelWidth(scrollText);

    if (scrollX < -textWidth) {
      scrollX = PANEL_RES_X;
    }
  }
}

// -----------------------------
// Mode 1: Statisches Logo / Startscreen
// -----------------------------
void drawLogoStatic() {
  unsigned long now = millis();

  if (now - lastFullRedraw < 500) {
    return;
  }

  lastFullRedraw = now;

  display->fillScreen(black);

  drawHeader();

  display->setTextSize(1);
  display->setTextColor(white);
  display->setCursor(5, 18);
  display->print("MATRIX");

  display->drawRect(0, 0, 64, 32, green);
  display->drawPixel(1, 1, blue);
  display->drawPixel(62, 1, blue);
  display->drawPixel(1, 30, blue);
  display->drawPixel(62, 30, blue);
}

// -----------------------------
// Mode 2: Pixel-Art Beispiel
// -----------------------------
void drawPixelArt() {
  unsigned long now = millis();

  if (now - lastFullRedraw < 500) {
    return;
  }

  lastFullRedraw = now;

  display->fillScreen(black);

  // Einfaches Pixel-Art Herz links
  display->fillRect(8, 8, 4, 4, red);
  display->fillRect(16, 8, 4, 4, red);
  display->fillRect(4, 12, 20, 4, red);
  display->fillRect(8, 16, 12, 4, red);
  display->fillRect(12, 20, 4, 4, red);

  // Text rechts
  display->setTextWrap(false);
  display->setTextSize(1);
  display->setTextColor(green);
  display->setCursor(31, 7);
  display->print("PIX");

  display->setTextColor(blue);
  display->setCursor(31, 18);
  display->print("ART");
}

// -----------------------------
// Mode 3: Random Pixel FX
// -----------------------------
void drawRandomFx() {
  unsigned long now = millis();

  // alle paar Sekunden Bildschirm löschen
  if (now - lastFullRedraw > 3000) {
    lastFullRedraw = now;
    display->fillScreen(black);

    display->setTextWrap(false);
    display->setTextSize(1);
    display->setTextColor(green);
    display->setCursor(3, 3);
    display->print("RANDOM");

    display->setTextColor(blue);
    display->setCursor(18, 14);
    display->print("FX");
  }

  if (now - lastFxUpdate >= 20) {
    lastFxUpdate = now;

    int x = random(0, PANEL_RES_X);
    int y = random(0, PANEL_RES_Y);

    uint16_t color;

    int r = random(0, 4);
    if (r == 0) color = green;
    else if (r == 1) color = blue;
    else if (r == 2) color = white;
    else color = yellow;

    display->drawPixel(x, y, color);
  }
}

// -----------------------------
// Demo Mode Switch
// -----------------------------
void handleAutoModeDemo() {
  if (!autoModeDemo) {
    return;
  }

  unsigned long now = millis();

  if (now - lastModeChange >= modeInterval) {
    lastModeChange = now;

    int nextMode = (int)currentMode + 1;

    if (nextMode > MODE_RANDOM_FX) {
      nextMode = MODE_SCROLL_TEXT;
    }

    setMode((DisplayMode)nextMode);
  }
}

// -----------------------------
// Serielle Steuerung
// -----------------------------
void handleSerialCommands() {
  if (!Serial.available()) {
    return;
  }

  char cmd = Serial.read();

  if (cmd == '0') {
    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT);
  } else if (cmd == '1') {
    autoModeDemo = false;
    setMode(MODE_LOGO_STATIC);
  } else if (cmd == '2') {
    autoModeDemo = false;
    setMode(MODE_PIXEL_ART);
  } else if (cmd == '3') {
    autoModeDemo = false;
    setMode(MODE_RANDOM_FX);
  } else if (cmd == 'a' || cmd == 'A') {
    autoModeDemo = true;
    lastModeChange = millis();
    Serial.println("Auto mode demo enabled");
  }
}

// -----------------------------
// Setup / Loop
// -----------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("SMARTFIX MATRIX");
  Serial.print("Firmware v");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("Mode System enabled");
  Serial.println("Serial commands:");
  Serial.println("0 = Scroll Text");
  Serial.println("1 = Static Logo");
  Serial.println("2 = Pixel Art");
  Serial.println("3 = Random FX");
  Serial.println("A = Auto Demo");
  Serial.println("================================");

  randomSeed(esp_random());

  initMatrix();

  Serial.println("Matrix initialized OK");

  setMode(MODE_SCROLL_TEXT);
  lastModeChange = millis();
}

void loop() {
  handleSerialCommands();
  handleAutoModeDemo();

  switch (currentMode) {
    case MODE_SCROLL_TEXT:
      drawScrollingText();
      break;

    case MODE_LOGO_STATIC:
      drawLogoStatic();
      break;

    case MODE_PIXEL_ART:
      drawPixelArt();
      break;

    case MODE_RANDOM_FX:
      drawRandomFx();
      break;
  }
}