#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobot_BMI160.h>
#include <math.h>
#include "frames.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DFRobot_BMI160 bmi160;

struct Animation {
  const uint8_t (*frames)[FRAME_BYTES];
  uint16_t frameCount;
  uint16_t frameDelay;
};

enum AnimationId : uint8_t {
  ANIM_SLEEPY = 0,
  ANIM_FREAKY = 1
};

const Animation animations[] = {
  { sleepy, SLEEPY_FRAME_COUNT, 120 },
  { freaky, FREAKY_FRAME_COUNT, 70 }
};

uint8_t currentAnimation = ANIM_SLEEPY;
uint16_t currentFrame = 0;

unsigned long lastFrameTime = 0;
unsigned long lastMotionRead = 0;
unsigned long freakyStartedAt = 0;
unsigned long lastTriggerAt = 0;

const unsigned long MOTION_READ_DELAY = 40;
const unsigned long FREAKY_DURATION = 3000;
const unsigned long TRIGGER_COOLDOWN = 2000;

float lastAx = 0.0;
float lastAy = 0.0;
float lastAz = 1.0;

// Lower value = easier to trigger
const float TAP_JERK_THRESHOLD = 1;

// Keeping shake higher so normal movement does not trigger too easily
const float SHAKE_G_THRESHOLD = 2;

void setAnimation(uint8_t newAnimation) {
  if (currentAnimation == newAnimation) {
    return;
  }

  currentAnimation = newAnimation;
  currentFrame = 0;
  lastFrameTime = 0;

  if (newAnimation == ANIM_FREAKY) {
    freakyStartedAt = millis();
  }
}

bool setupBMI160() {
  if (bmi160.I2cInit(0x68) == BMI160_OK) {
    Serial.println("BMI160 found at 0x68");
    return true;
  }

  if (bmi160.I2cInit(0x69) == BMI160_OK) {
    Serial.println("BMI160 found at 0x69");
    return true;
  }

  Serial.println("BMI160 not found");
  return false;
}

void primeMotionBaseline() {
  int16_t accelGyro[6] = {0};

  if (bmi160.getAccelGyroData(accelGyro) == BMI160_OK) {
    lastAx = accelGyro[3] / 16384.0;
    lastAy = accelGyro[4] / 16384.0;
    lastAz = accelGyro[5] / 16384.0;
  }
}

void checkTapOrShake() {
  unsigned long now = millis();

  if (now - lastMotionRead < MOTION_READ_DELAY) {
    return;
  }

  lastMotionRead = now;

  int16_t accelGyro[6] = {0};

  if (bmi160.getAccelGyroData(accelGyro) != BMI160_OK) {
    return;
  }

  float ax = accelGyro[3] / 16384.0;
  float ay = accelGyro[4] / 16384.0;
  float az = accelGyro[5] / 16384.0;

  float accelMag = sqrt((ax * ax) + (ay * ay) + (az * az));

  float jerk =
    fabs(ax - lastAx) +
    fabs(ay - lastAy) +
    fabs(az - lastAz);

  lastAx = ax;
  lastAy = ay;
  lastAz = az;

  bool tapDetected = jerk > TAP_JERK_THRESHOLD;
  bool shakeDetected = fabs(accelMag - 1.0) > SHAKE_G_THRESHOLD;

  if ((tapDetected || shakeDetected) && now - lastTriggerAt > TRIGGER_COOLDOWN) {
    lastTriggerAt = now;

    Serial.print("Tap/shake detected | Jerk: ");
    Serial.print(jerk, 3);
    Serial.print(" | G: ");
    Serial.println(accelMag, 3);

    setAnimation(ANIM_FREAKY);
  }
}

void handleEmotionState() {
  if (currentAnimation == ANIM_FREAKY) {
    if (millis() - freakyStartedAt >= FREAKY_DURATION) {
      setAnimation(ANIM_SLEEPY);
    }
  }
}

void playCurrentAnimation() {
  const Animation& anim = animations[currentAnimation];
  unsigned long now = millis();

  if (now - lastFrameTime < anim.frameDelay) {
    return;
  }

  lastFrameTime = now;

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
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED failed");
    while (true);
  }

  display.clearDisplay();
  display.display();

  setupBMI160();

  delay(500);
  primeMotionBaseline();

  currentAnimation = ANIM_SLEEPY;
  currentFrame = 0;
}

void loop() {
  checkTapOrShake();
  handleEmotionState();
  playCurrentAnimation();
}