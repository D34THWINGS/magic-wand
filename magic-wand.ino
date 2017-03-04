#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>
#include <SD.h>

#define PIXEL_PIN      2
#define PIXEL_COUNT    144
#define SD_PIN         4
#define BACKLIGHT_PIN  10
#define DIM_TIMER      5000
#define MENU_DELAY     40

#define btnRIGHT       0
#define btnUP          1
#define btnDOWN        2
#define btnLEFT        3
#define btnSELECT      4
#define btnNONE        5

#define minDelayValue  0
#define maxDelayValue  100
#define stepDelayValue 1
#define minStartDelay  0
#define maxStartDelay  5000
#define stepStartDelay 100
#define minBrightness  0
#define maxBrightness  255
#define stepBrightness 1

File loadedFile;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);
LiquidCrystal lcd(8, 9, 3, 5, 6, 7);

byte curY = 0;
unsigned long lastButtonClick = 1;
unsigned long lastFrame = 0;
byte lastKey = btnNONE;
byte delayValue = 10;
byte brightness = 20;
short startDelay = 0;
bool loopAnimation = false;
bool animationOn = false;
byte colorEncoding = 8;
byte currentFileIndex = 0;
char currentFileName[10];

// ============== SETUP ==============

void setup() {
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
  lcd.begin(16, 2);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  setupSDcard();
  menu(btnNONE);
}

// ============== MAIN LOOP ==============

void loop() {
  unsigned long currentTime = millis();

  byte newKey = readLcdButtons();

  if (newKey != btnNONE) {
    menu(newKey);
    if (curY != 1) {
      lastButtonClick = currentTime;
    }
  }

  lastKey = newKey;

  if (currentTime - lastFrame < delayValue) {
    return;
  }

  if (animationOn) {
    if (!loadedFile.available()) {
      animationOn = loopAnimation;
      loadedFile.seek(0);
      if (loopAnimation) {
        sdCardPattern();
      } else {
        loadedFile.close();
        loadManifest();
        clearStrip();
        menu(btnNONE);
      }
    } else {
      sdCardPattern();
    }
  } else {
    digitalWrite(BACKLIGHT_PIN, (currentTime - lastButtonClick < DIM_TIMER) && (lastButtonClick != 0) ? HIGH : LOW);
    delay(MENU_DELAY);
  }

  lastFrame = currentTime;
}

// ============== LCD SCREEN METHODS ==============

void menu(byte newKey) {
  if (newKey != lastKey) {
    if (newKey == btnDOWN) {
      curY++;
    }
    if (newKey == btnUP && curY > 0) {
      curY--;
    }
  }

  lcd.clear();
  switch(curY) {
    case 0:
      selectFile(newKey);
      break;
    case 1:
      toggleAnimation(newKey);
      break;
    case 2:
      setFrameRate(newKey);
      break;
    case 3:
      setStartDelay(newKey);
      break;
    case 4:
      toggleLooping(newKey);
      break;
    case 5:
      setLedBrightness(newKey);
      break;
    case 6:
      toggleEncoding(newKey);
      break;
    default:
      curY--;
      break;
  }
}

void selectFile(byte newKey) {
  lcd.print(F("File to load:"));
  if (newKey == btnSELECT && newKey != lastKey) {
    loadNextFile();
  }
  lcd.setCursor(0, 1);
  lcd.print(currentFileName);
}

void toggleAnimation(byte newKey) {
  lcd.print(animationOn ? F("Stop") : F("Start"));
  if (newKey != lastKey && newKey == btnSELECT) {
    lastButtonClick = 0;
    animationOn = !animationOn;
    digitalWrite(BACKLIGHT_PIN, LOW);
    clearStrip();
    if (animationOn) {
      loadedFile.close();
      loadedFile = SD.open("/IMAGES/" + String(currentFileIndex) + ".MWT");
      delay(startDelay);
    } else {
      loadedFile.close();
      loadManifest();
    }
  }
}

void setFrameRate(byte newKey) {
  lcd.print(F("Frame delay:"));
  if (newKey == btnLEFT && delayValue > minDelayValue) {
    delayValue -= stepDelayValue;
  }
  if (newKey == btnRIGHT && delayValue < maxDelayValue) {
    delayValue += stepDelayValue;
  }
  lcd.setCursor(0, 1);
  lcd.print(delayValue);
}

void setStartDelay(byte newKey) {
  lcd.print(F("Start delay:"));
  if (newKey == btnLEFT && startDelay > minStartDelay) {
    startDelay -= stepStartDelay;
  }
  if (newKey == btnRIGHT && startDelay < maxStartDelay) {
    startDelay += stepStartDelay;
  }
  lcd.setCursor(0, 1);
  lcd.print(startDelay);
}

void toggleLooping(byte newKey) {
  lcd.print(F("Loop:"));
  if (newKey == btnLEFT) {
    loopAnimation = false;
  }
  if (newKey == btnRIGHT) {
    loopAnimation = true;
  }
  lcd.setCursor(0, 1);
  lcd.print(loopAnimation ? F("yes") : F("no"));
}

void toggleEncoding(byte newKey) {
  lcd.print(F("Encoding:"));
  if (newKey == btnLEFT) {
    colorEncoding = 8;
  }
  if (newKey == btnRIGHT) {
    colorEncoding = 12;
  }
  lcd.setCursor(0, 1);
  lcd.print(colorEncoding == 8 ? F("8bit") : F("12bit"));
}

void setLedBrightness(byte newKey) {
  lcd.print(F("Brightness:"));
  if (newKey == btnLEFT && brightness > minBrightness) {
    brightness -= stepBrightness;
  }
  if (newKey == btnRIGHT && brightness < maxBrightness) {
    brightness += stepBrightness;
  }
  lcd.setCursor(0, 1);
  lcd.print(brightness);
}

// ============== STRIP METHODS ==============

void clearStrip() {
  for (byte i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();
}

void read12bit() {
  for (byte i = 0; i < strip.numPixels(); i += 2) {
    byte firstTwo = loadedFile.read();
    byte middleTwo = loadedFile.read();
    byte lastTwo = loadedFile.read();
    strip.setPixelColor(i, strip.Color(lowOrder(firstTwo), highOrder(firstTwo), lowOrder(middleTwo)));
    strip.setPixelColor(i + 1, strip.Color(highOrder(middleTwo), lowOrder(lastTwo), highOrder(lastTwo)));
  }
}

void read8bit() {
  for (byte i = 0; i < strip.numPixels(); i++) {
    byte color = loadedFile.read();
    byte red = color & 0b11100000;
    byte green = (color & 0b11100) << 3;
    byte blue = (color & 0b11) << 5;
    strip.setPixelColor(i, red, green, blue);
  }
}

void sdCardPattern() {
  if (colorEncoding == 8) {
    read8bit();
  } else {
    read12bit();
  }
  strip.show();
}

byte lowOrder(byte b) {
  return (b & 0b1111) << 4;
}

byte highOrder(byte b) {
  return b & 0b11110000;
}

byte readLcdButtons() {
  short adcKeyIn = analogRead(0);

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

// ============== SD CARD METHODS ==============

void setupSDcard() {
  pinMode(SD_PIN, OUTPUT);
  while (!SD.begin(SD_PIN)) {
    delay(1000);
  }
  loadManifest();
}

void loadManifest() {
  loadedFile = SD.open(F("/IMAGES/MANIFEST"));
  if (currentFileIndex != 0) {
    // Reload filename of selected file
    byte tmp = currentFileIndex;
    currentFileIndex = 0;
    while(tmp != currentFileIndex) {
      loadNextFile();
    }
  } else {
    loadNextFile();
  }
}

void loadNextFile() {
  byte i = 0;
  do {
    char nextChar = loadedFile.read();
    if (nextChar == -1) {
      currentFileIndex = 0;
      loadedFile.seek(0);
      loadNextFile();
      return;
    }
    if (nextChar == '\n') {
      for (;i < 9; i++) {
        currentFileName[i] = ' ';
      }
      break;
    }
    currentFileName[i] = nextChar;
    i++;
  } while(loadedFile.available());
  currentFileIndex++;
}

