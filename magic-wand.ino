// This is a demonstration on how to use an input device to trigger changes on your neo pixels.
// You should wire a momentary push button to connect from ground to a digital IO pin.  When you
// press the button it will change to a new pixel animation.  Note that you need to press the
// button once to start the first animation!

#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

#define PIXEL_PIN     3
#define PIXEL_COUNT   144

#define btnRIGHT      0
#define btnUP         1
#define btnDOWN       2
#define btnLEFT       3
#define btnSELECT     4
#define btnNONE       5

#define NB_COLORS     8
#define NB_MENUS      6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

uint16_t showType = 0;
uint16_t delayValue = 30;
uint16_t frame = 0;
int lcdKey = 0;
int oldLcdKey = 0;
int adcKeyIn = 0;
int backLightPin = 10;
String menuNames[NB_MENUS] = {"Off", "Color wipe", "Theater chase", "Rainbow", "Speed", "Brightness"};
int menus[4] = {0};
uint32_t selectedColor = strip.Color(0, 0, 0);
uint32_t rainbowMode = 0;
long lastFrame = 0;
long lastButtonClick = 0;
uint32_t colors[NB_COLORS] = {strip.Color(0, 0, 0, 255), strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255), strip.Color(255, 0, 255), strip.Color(255, 75, 0), strip.Color(0, 255, 255), strip.Color(255, 150, 0)};
String colorNames[] = {"white", "red", "green", "blue", "pink", "orange", "cyan", "yellow"};
uint8_t brightness = 255;

void setup() {
  lcd.begin(16, 2);
  pinMode(backLightPin, OUTPUT);
  digitalWrite(backLightPin, HIGH);
  strip.begin();
  strip.show();
  menu();
}

void loop() {
  delay(10);
  long currentTime = millis();

  // Get current button state.
  lcdKey = readLcdButtons();

  // Check if state changed from high to low (button press).
  if (lcdKey != btnNONE) {
    menu();
    lastButtonClick = currentTime;
  }

  oldLcdKey = lcdKey;

  digitalWrite(backLightPin, currentTime - lastButtonClick > 5000 ? LOW : HIGH);

  if (currentTime - lastFrame < delayValue) {
    return;
  }
  
  switch (showType) {
    case 0:
      colorWipe(strip.Color(0, 0, 0), delayValue);    // Black/off
      delay(40); // Lowering frame rate while off
      break;
    case 1: colorWipe(selectedColor, delayValue);
      break;
    case 2: theaterChase(selectedColor, delayValue);
      break;
    case 3: rainbowMode == 0 ? rainbow(20) : rainbowMode == 1 ? rainbowCycle(20) : theaterChaseRainbow(delayValue);
      break;
  }

  frame++;
  lastFrame = currentTime;
}

void menu() {
  menus[0] = menuAxisSingle(menus[0], 0, NB_MENUS - 1, btnUP, btnDOWN);

  if (lcdKey == btnSELECT && oldLcdKey == btnNONE && menus[0] <= 3) {
    frame = 0;
    selectedColor = colors[menus[1]];
    rainbowMode = menus[2];
    showType = menus[0];
  }

  lcd.clear();
  lcd.print(menuNames[menus[0]]);

  if (menus[0] == 1 || menus[0] == 2) {
    menus[1] = menuAxisSingle(menus[1], 0, NB_COLORS - 1);
    lcd.setCursor(0, 1);
    String text = "> Color: " + colorNames[menus[1]];
    lcd.print(text);
  }

  if (menus[0] == 3) {
    menus[2] = menuAxisSingle(menus[2], 0, 2);
    lcd.setCursor(0, 1);
    String text = "> Mode: " + (String)(menus[2] == 0 ? "partial" : menus[2] == 1 ? "full" : "chase");
    lcd.print(text);
  }

  if (menus[0] == 4) {
    delayValue = menuAxis(delayValue, 10, 3000);
    lcd.setCursor(0, 1);
    String text = "> " + (String)delayValue;
    lcd.print(text);
  }

  if (menus[0] == 5) {
    brightness = menuAxis(brightness, 0, 255);
    lcd.setCursor(0, 1);
    String text = "> " + (String)brightness;
    lcd.print(text);
    strip.setBrightness(brightness);
  }
}

int menuAxis(int currentValue, short minValue, short maxValue, int up, int down) {
  if (lcdKey == up) {
    currentValue--;
    if (currentValue < minValue) currentValue = maxValue;
  }
  if (lcdKey == down) {
    currentValue++;
    if (currentValue > maxValue) currentValue = minValue;
  }
  return currentValue;
}

int menuAxis(int currentValue, short minValue, short maxValue) {
  return menuAxis(currentValue, minValue, maxValue, btnLEFT, btnRIGHT);
}

int menuAxisSingle(int currentValue, short minValue, short maxValue, int up, int down) {
  return oldLcdKey == btnNONE ? menuAxis(currentValue, minValue, maxValue, up, down) : currentValue;
}

int menuAxisSingle(int currentValue, short minValue, short maxValue) {
  return menuAxisSingle(currentValue, minValue, maxValue, btnLEFT, btnRIGHT);
}

void clearStrip() {
  fullColor(strip.Color(0, 0, 0));
}

void fullColor(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  uint16_t i = frame % (strip.numPixels() + 1);

  if (i == 0) {
    clearStrip();
  } else {
    strip.setPixelColor(i - 1, c);
    strip.show();
  }
}

void rainbow(uint8_t wait) {
  uint16_t j = frame % 256;

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel((i + j) & 255));
  }
  
  strip.show();
}

void rainbowCycle(uint8_t wait) {
  uint16_t j = frame % 256;

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
  }
  
  strip.show();
}

void theaterChase(uint32_t c, uint8_t wait) {
  int q = frame % 3;
  
  for (int i = 0; i < strip.numPixels(); i = i + 3) {
    strip.setPixelColor(i + q, c);  //turn every third pixel on
  }
  
  strip.show();

  for (int i = 0; i < strip.numPixels(); i = i + 3) {
    strip.setPixelColor(i + q, 0);      //turn every third pixel off
  }
}

void theaterChaseRainbow(uint8_t wait) {
  int q = frame % 3;
  int j = frame % 256;
  
  for (int i = 0; i < strip.numPixels(); i = i + 3) {
    strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
  }
  
  strip.show();

  for (int i = 0; i < strip.numPixels(); i = i + 3) {
    strip.setPixelColor(i + q, 0);      //turn every third pixel off
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

int readLcdButtons() {
  adcKeyIn = analogRead(0);

  if (adcKeyIn < 50)
      return btnRIGHT;
  if (adcKeyIn < 195)
      return btnUP;
  if (adcKeyIn < 380)
      return btnDOWN;
  if (adcKeyIn < 555)
      return btnLEFT;
  if (adcKeyIn < 790)
      return btnSELECT;
  if (adcKeyIn > 1000)
      return btnNONE;

  return btnNONE;
}


