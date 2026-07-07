#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

MatrixPanel_I2S_DMA *display = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("================================");
  Serial.println("SMARTFIX MATRIX DISPLAY TEST 002");
  Serial.println("================================");

  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN
  );

  // Waveshare ESP32-S3-RGB-Matrix Board Settings
  mxconfig.gpio.e = 9;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

  display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!display->begin()) {
    Serial.println("ERROR: display->begin() failed!");
    return;
  }

  Serial.println("Matrix initialized OK");

  display->setBrightness8(80);
  display->clearScreen();

  Serial.println("Starting color test...");
}

void loop() {
  uint16_t red   = display->color565(255, 0, 0);
  uint16_t green = display->color565(0, 255, 0);
  uint16_t blue  = display->color565(0, 0, 255);
  uint16_t white = display->color565(255, 255, 255);
  uint16_t black = display->color565(0, 0, 0);

  Serial.println("RED");
  display->fillScreen(red);
  delay(1500);

  Serial.println("GREEN");
  display->fillScreen(green);
  delay(1500);

  Serial.println("BLUE");
  display->fillScreen(blue);
  delay(1500);

  Serial.println("WHITE");
  display->fillScreen(white);
  delay(1500);

  Serial.println("TEXT");
  display->fillScreen(black);

  display->setTextWrap(false);
  display->setTextSize(1);

  display->setCursor(3, 4);
  display->setTextColor(green);
  display->print("Smart");

  display->setCursor(36, 4);
  display->setTextColor(blue);
  display->print("Fix");

  display->setCursor(8, 18);
  display->setTextColor(white);
  display->print("MATRIX");

  delay(3000);
}