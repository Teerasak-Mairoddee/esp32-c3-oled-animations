#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "frames.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct Animation {
  const uint8_t (*frames)[FRAME_BYTES];
  uint16_t frameCount;
  uint16_t frameDelay;
  uint8_t loopsBeforeNext;
};

const Animation animations[] = {
  { freaky,  FREAKY_FRAME_COUNT,  100, 5 },
  { sleepy, SLEEPY_FRAME_COUNT, 120,  13 },
};

const uint8_t animationCount = sizeof(animations) / sizeof(animations[0]);

uint8_t currentAnimation = 0;
uint16_t currentFrame = 0;
uint8_t currentLoop = 0;

unsigned long lastFrameTime = 0;

void setup() {
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true);
  }

  display.clearDisplay();
  display.display();
}

void loop() {
  const Animation& anim = animations[currentAnimation];

  if (millis() - lastFrameTime >= anim.frameDelay) {
    lastFrameTime = millis();

    display.clearDisplay();

    display.drawBitmap(
      0,
      0,
      anim.frames[currentFrame],
      FRAME_WIDTH,
      FRAME_HEIGHT,
      SSD1306_WHITE
    );

    display.display();

    currentFrame++;

    if (currentFrame >= anim.frameCount) {
      currentFrame = 0;
      currentLoop++;

      if (currentLoop >= anim.loopsBeforeNext) {
        currentLoop = 0;
        currentAnimation++;

        if (currentAnimation >= animationCount) {
          currentAnimation = 0;
        }
      }
    }
  }
}