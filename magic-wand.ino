#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>

#define PIXEL_PIN     2
#define PIXEL_COUNT   144
#define SD_PIN        4
#define BACKLIGHT_PIN 10
#define DIM_TIMER     5000

#define btnRIGHT      0
#define btnUP         1
#define btnDOWN       2
#define btnLEFT       3
#define btnSELECT     4
#define btnNONE       5

File root;
File loadedFile;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);
LiquidCrystal lcd(8, 9, 3, 5, 6, 7);

byte curX = 0;
byte curY = 0;
long lastButtonClick = 0;
long lastFrame = 0;
byte lastKey = btnNONE;
long delayValue = 10;
long startDelay = 0;
bool loopAnimation = false;
bool animationOn = false;
byte colorEncoding = 8;
long numberOfFiles = 0;

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show();
  strip.setBrightness(16);
  lcd.begin(16, 2);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  setupSDcard();
  menu(btnNONE);
}

void loop() {
  long currentTime = millis();

  long newKey = readLcdButtons();

  if (newKey != btnNONE) {
    menu(newKey);
    lastButtonClick = currentTime;
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
        clearStrip();
        menu(btnNONE);
      }
    } else {
      sdCardPattern();
    }
  } else {
    digitalWrite(BACKLIGHT_PIN, currentTime - lastButtonClick > DIM_TIMER ? LOW : HIGH);
    delay(40);
  }

  lastFrame = currentTime;
}

void menu(long newKey) {
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
      lcd.print("File to load:");
      if (newKey == btnSELECT && newKey != lastKey) {
        loadNextFile();
      }
      lcd.setCursor(0, 1);
      lcd.print(loadedFile.name());
      break;
    case 1:
      lcd.print(animationOn ? "Stop" : "Start");
      if (loadedFile.available()) {
        if (newKey != lastKey && newKey == btnSELECT) {
          animationOn = !animationOn;
          digitalWrite(BACKLIGHT_PIN, LOW);
          clearStrip();
          if (animationOn) {
            loadedFile.seek(0);
            delay(startDelay);
          }
        }
      } else {
        lcd.setCursor(0, 1);
        lcd.print("!Select file!");
      }
      break;
    case 2:
      lcd.print("Frame delay:");
      if (newKey == btnLEFT && delayValue > 0) {
        delayValue -= 1;
      }
      if (newKey == btnRIGHT && delayValue < 100) {
        delayValue += 1;
      }
      lcd.setCursor(0, 1);
      lcd.print("> " + (String)delayValue);
      break;
    case 3:
      lcd.print("Start delay:");
      if (newKey == btnLEFT && startDelay > 0) {
        startDelay -= 100;
      }
      if (newKey == btnRIGHT && startDelay < 5000) {
        startDelay += 100;
      }
      lcd.setCursor(0, 1);
      lcd.print("> " + (String)startDelay);
      break;
    case 4:
      lcd.print("Loop:");
      if (newKey == btnLEFT) {
        loopAnimation = false;
      }
      if (newKey == btnRIGHT) {
        loopAnimation = true;
      }
      lcd.setCursor(0, 1);
      lcd.print("> " + (String)(loopAnimation ? "yes" : "no"));
      break;
    case 5:
      lcd.print("Encoding:");
      if (newKey == btnLEFT) {
        colorEncoding = 8;
      }
      if (newKey == btnRIGHT) {
        colorEncoding = 12;
      }
      lcd.setCursor(0, 1);
      lcd.print("> " + (String)(colorEncoding == 8 ? "8bit" : "12bit"));
      break;
    default:
      curY--;
      break;
  }
}

void clearStrip() {
  for (short i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();
}

void read12bit() {
  for (short i = 0; i < strip.numPixels(); i += 2) {
    byte firstTwo = loadedFile.read();
    byte middleTwo = loadedFile.read();
    byte lastTwo = loadedFile.read();
    strip.setPixelColor(i, strip.Color(lowOrder(firstTwo), highOrder(firstTwo), lowOrder(middleTwo)));
    strip.setPixelColor(i + 1, strip.Color(highOrder(middleTwo), lowOrder(lastTwo), highOrder(lastTwo)));
  }
}

void read8bit() {
  for (short i = 0; i < strip.numPixels(); i++) {
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
  long adcKeyIn = analogRead(0);

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

void setupSDcard() {
  pinMode(SD_PIN, OUTPUT);
  while (!SD.begin(SD_PIN)) {
    delay(1000);
  }
  root = SD.open("/images");
  loadNextFile();
}

void loadNextFile() {
  loadNextFile(0);
}

void loadNextFile(int recur) {
  if (loadedFile) {
    loadedFile.close();
  }
  loadedFile = root.openNextFile();
  if (!loadedFile && recur < 1) {
    root.rewindDirectory();
    return loadNextFile(recur++);
  }
  if (loadedFile.isDirectory()) {
    return loadNextFile();
  }
}

