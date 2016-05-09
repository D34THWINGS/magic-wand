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

File loadedFile;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);
LiquidCrystal lcd(8, 9, 3, 5, 6, 7);

long curX = 0;
long curY = 0;
long lastButtonClick = 0;
long lastFrame = 0;
long lastKey = btnNONE;
long frameRate = 80;
long startDelay = 0;
bool loopAnimation = false;
bool animationOn = false;

void setup() {
  SD.begin(SD_PIN);
  lcd.begin(16, 2);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  strip.begin();
  strip.show();
  menu(btnNONE);
  strip.setBrightness(1);
  strip.show();
}

void loop() {
  long currentTime = millis();

  long newKey = readLcdButtons();

  if (newKey != btnNONE) {
    menu(newKey);
    lastButtonClick = currentTime;
  }

  lastKey = newKey;

  digitalWrite(BACKLIGHT_PIN, (currentTime - lastButtonClick > DIM_TIMER) || animationOn ? LOW : HIGH);

  long delayValue = 1000 / frameRate;
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
      }
    } else {
      sdCardPattern();
    }
  } else {
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
      if (newKey != lastKey && newKey == btnSELECT && !loadedFile.available()) {
        loadedFile = SD.open("test.mwt");
      }
      lcd.setCursor(0, 1);
      lcd.print(!loadedFile.available() ? "test.mwt" : "> test.mwt");
      break;
    case 1:
      lcd.print(animationOn ? "Stop" : "Start");
      if (loadedFile.available()) {
        if (newKey != lastKey && newKey == btnSELECT) {
          animationOn = !animationOn;
          clearStrip();
          if (animationOn) {
            delay(startDelay);
          }
        }
      } else {
        lcd.setCursor(0, 1);
        lcd.print("!Select file!");
      }
      break;
    case 2:
      lcd.print("Speed:");
      if (newKey == btnLEFT && frameRate > 10) {
        frameRate -= 1;
      }
      if (newKey == btnRIGHT && frameRate < 100) {
        frameRate += 1;
      }
      lcd.setCursor(0, 1);
      lcd.print("> " + (String)frameRate);
      break;
    case 3:
      lcd.print("Delay:");
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

void sdCardPattern() {
  for (short i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, readColorFromFile());
  }
  strip.show();
}

uint32_t readColorFromFile() {
  return strip.Color(loadedFile.read(), loadedFile.read(), loadedFile.read());
}

int readLcdButtons() {
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


