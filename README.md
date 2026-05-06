# ESP32-C3 OLED Animation Desk Buddy

This project displays custom frame-by-frame animations on an SSD1306 OLED screen using an ESP32-C3.

The animation frames are exported from Procreate, converted into 1-bit monochrome bitmap data, stored in `frames.h`, and played on the OLED using the Adafruit SSD1306 library.

The project has now been expanded with a BMI160 motion sensor so the desk buddy can react to movement. By default, the device plays a sleepy animation. When the device is tapped or shaken, it switches to the freaky animation for a short duration, then returns to sleepy.

## Project Overview

The current behaviour is:

```text
Default state: sleepy animation
Tap or shake detected: freaky animation
After timeout: return to sleepy animation
```

Current animations:

- `sleepy`
- `freaky`

Each animation is stored as a separate frame bank inside `frames.h`. The main Arduino sketch reads those frame banks and displays them one frame at a time.

## Hardware Used

- ESP32-C3 development board
- SSD1306 128x64 OLED display
- BMI160 accelerometer and gyroscope module
- Jumper wires
- USB cable for programming

## OLED Display Details

The code is configured for a standard 128x64 SSD1306 OLED display.

```cpp
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
```

Some OLED modules use address `0x3D` instead of `0x3C`. Change `OLED_ADDR` if the screen does not start.

## Motion Sensor Details

The project uses a BMI160 motion sensor.

The BMI160 is used to detect:

- Taps
- Shakes
- Sudden movement

The code reads accelerometer data from the BMI160 and calculates a simple jerk value.

```cpp
float jerk =
  fabs(ax - lastAx) +
  fabs(ay - lastAy) +
  fabs(az - lastAz);
```

If the jerk value passes the tap threshold, the desk buddy switches to the freaky animation.

## Folder Structure

```text
c3_custom_anim/
├── c3_custom_anim.ino
├── frames.h
└── README.md
```

## File Roles

### `c3_custom_anim.ino`

This is the main Arduino sketch.

It handles:

- Starting the OLED display
- Starting the BMI160 motion sensor
- Reading animation frame data
- Drawing frames to the screen
- Controlling frame speed
- Reading motion data
- Detecting tap or shake movement
- Switching from sleepy to freaky
- Returning back to sleepy after a timeout

### `frames.h`

This file contains the raw bitmap animation data.

Each frame is stored as a 1-bit monochrome bitmap. For a 128x64 OLED screen, each full frame uses 1024 bytes.

```text
128 x 64 pixels = 8192 pixels
8192 bits / 8 = 1024 bytes
```

## Required Arduino Libraries

Install these through the Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306
- DFRobot BMI160

You also need ESP32 board support installed in Arduino IDE.

## Board Selection

In Arduino IDE, select the correct ESP32-C3 board.

The exact board depends on your module, but common options include:

- ESP32C3 Dev Module
- Seeed XIAO ESP32C3
- LOLIN C3 Mini

Choose the board that matches your hardware.

## Shared I2C Wiring

The OLED and BMI160 both use I2C, so they can share the same SDA and SCL pins.

Current ESP32-C3 pin setup:

```cpp
#define SDA_PIN 8
#define SCL_PIN 9
```

### OLED Wiring

| OLED Pin | ESP32-C3 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |

### BMI160 Wiring

| BMI160 Pin | ESP32-C3 Pin |
|---|---|
| VIN | 3.3V |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |
| CS | 3.3V |
| SA0 | GND or 3.3V |

BMI160 address options:

```text
SA0 -> GND  = 0x68
SA0 -> 3.3V = 0x69
```

The code tries both addresses:

```cpp
if (bmi160.I2cInit(0x68) == BMI160_OK) {
  Serial.println("BMI160 found at 0x68");
  return true;
}

if (bmi160.I2cInit(0x69) == BMI160_OK) {
  Serial.println("BMI160 found at 0x69");
  return true;
}
```

## Current Animation System

The main sketch uses this animation structure:

```cpp
struct Animation {
  const uint8_t (*frames)[FRAME_BYTES];
  uint16_t frameCount;
  uint16_t frameDelay;
};
```

The animations are added to this list:

```cpp
const Animation animations[] = {
  { sleepy, SLEEPY_FRAME_COUNT, 120 },
  { freaky, FREAKY_FRAME_COUNT, 70 }
};
```

The values mean:

```text
animation frame bank, number of frames, delay per frame
```

Example:

```cpp
{ sleepy, SLEEPY_FRAME_COUNT, 120 }
```

This means:

- Play the `sleepy` frame bank
- Use the number of frames defined by `SLEEPY_FRAME_COUNT`
- Wait 120 ms between frames

## Current Emotion Logic

The project currently uses two animation states:

```cpp
enum AnimationId : uint8_t {
  ANIM_SLEEPY = 0,
  ANIM_FREAKY = 1
};
```

The desk buddy starts in sleepy mode:

```cpp
uint8_t currentAnimation = ANIM_SLEEPY;
```

When a tap or shake is detected, the animation changes to freaky:

```cpp
setAnimation(ANIM_FREAKY);
```

After the freaky duration expires, it returns to sleepy:

```cpp
if (millis() - freakyStartedAt >= FREAKY_DURATION) {
  setAnimation(ANIM_SLEEPY);
}
```

## Motion Timing Settings

These constants control how the motion interaction feels.

```cpp
const unsigned long MOTION_READ_DELAY = 40;
const unsigned long FREAKY_DURATION = 3000;
const unsigned long TRIGGER_COOLDOWN = 500;
```

### `MOTION_READ_DELAY`

```cpp
const unsigned long MOTION_READ_DELAY = 40;
```

Controls how often the ESP32 reads the BMI160.

```text
40 ms = about 25 motion checks per second
```

Lower values make it react faster.

Example:

```cpp
const unsigned long MOTION_READ_DELAY = 20;
```

Higher values reduce how often the sensor is checked.

Example:

```cpp
const unsigned long MOTION_READ_DELAY = 100;
```

### `FREAKY_DURATION`

```cpp
const unsigned long FREAKY_DURATION = 3000;
```

Controls how long the freaky animation plays after a tap or shake.

```text
3000 ms = 3 seconds
```

Longer freaky reaction:

```cpp
const unsigned long FREAKY_DURATION = 5000;
```

Shorter freaky reaction:

```cpp
const unsigned long FREAKY_DURATION = 1500;
```

### `TRIGGER_COOLDOWN`

```cpp
const unsigned long TRIGGER_COOLDOWN = 500;
```

Prevents one physical tap from triggering the animation repeatedly.

```text
500 ms = half a second cooldown
```

More responsive:

```cpp
const unsigned long TRIGGER_COOLDOWN = 250;
```

Less spammy:

```cpp
const unsigned long TRIGGER_COOLDOWN = 1000;
```

## Sensitivity Settings

Current sensitivity values:

```cpp
const float TAP_JERK_THRESHOLD = 0.05;
const float SHAKE_G_THRESHOLD = 0.55;
```

### Tap Sensitivity

```cpp
const float TAP_JERK_THRESHOLD = 0.05;
```

Lower value means easier tap detection.

More sensitive:

```cpp
const float TAP_JERK_THRESHOLD = 0.03;
```

Less sensitive:

```cpp
const float TAP_JERK_THRESHOLD = 0.08;
```

### Shake Sensitivity

```cpp
const float SHAKE_G_THRESHOLD = 0.55;
```

Lower value means easier shake detection.

More sensitive:

```cpp
const float SHAKE_G_THRESHOLD = 0.35;
```

Less sensitive:

```cpp
const float SHAKE_G_THRESHOLD = 0.75;
```

## Motion Debugging Steps Used

### 1. I2C Scanner

An I2C scanner was used to confirm that devices were visible on the bus.

Expected results:

```text
0x3C = OLED
0x68 or 0x69 = BMI160
```

At first, only the OLED appeared:

```text
I2C Devices:
Found: 0x3C
```

The wiring was then corrected so the BMI160 could be detected.

### 2. BMI160 Raw Data Test

A raw BMI160 test was used to confirm that gyro and accelerometer data could be read.

The test displayed values such as:

```text
GX
GY
GZ
AX
AY
AZ
Total G
```

When sitting still, total G should be close to:

```text
1.00
```

When tapped or moved, the values should change.

### 3. Tap and Shake Debug Test

A simple text debug screen was used before connecting the logic back to the real animations.

The screen showed:

```text
SLEEPY
FREAKY
Jerk value
Tap state
Shake state
```

This helped tune the tap sensitivity before returning to the real `sleepy` and `freaky` animations.

## How To Add A New Animation

### 1. Convert your animation frames

Create or export your animation from Procreate.

Recommended export process:

```text
Procreate animation
-> GIF or PNG sequence
-> 128x64 black and white frames
-> C bitmap array
```

You can convert frames using:

- image2cpp
- a custom Python converter
- another bitmap-to-C-array tool

The OLED works best with white drawings on a black background.

### 2. Add the new frame count in `frames.h`

Example:

```cpp
#define HAPPY_FRAME_COUNT 8
```

### 3. Add the new frame array in `frames.h`

```cpp
const uint8_t happy[HAPPY_FRAME_COUNT][FRAME_BYTES] PROGMEM = {
  {
    // frame 0 data
  },
  {
    // frame 1 data
  }
};
```

Each 128x64 frame should contain 1024 bytes.

### 4. Add the animation to `c3_custom_anim.ino`

Update the animation ID list:

```cpp
enum AnimationId : uint8_t {
  ANIM_SLEEPY = 0,
  ANIM_FREAKY = 1,
  ANIM_HAPPY = 2
};
```

Add the animation to the animation list:

```cpp
const Animation animations[] = {
  { sleepy, SLEEPY_FRAME_COUNT, 120 },
  { freaky, FREAKY_FRAME_COUNT, 70 },
  { happy,  HAPPY_FRAME_COUNT,  90 }
};
```

Then trigger it from your logic:

```cpp
setAnimation(ANIM_HAPPY);
```

## Important Notes

### Use `PROGMEM`

Animation data can become large. `PROGMEM` stores the frames in flash memory instead of RAM.

Example:

```cpp
const uint8_t sleepy[SLEEPY_FRAME_COUNT][FRAME_BYTES] PROGMEM = {
```

### Frame size must match the OLED

For a 128x64 screen:

```cpp
#define FRAME_BYTES 1024
```

If frames were converted at a different size, the animation may fail or display incorrectly.

### Avoid using one shared `FRAME_COUNT`

Each animation should have its own frame count.

Good:

```cpp
#define FREAKY_FRAME_COUNT 5
#define SLEEPY_FRAME_COUNT 13
```

Avoid:

```cpp
#define FRAME_COUNT 5
```

A single shared frame count causes errors when different animations have different numbers of frames.

## Common Errors

### `'FRAME_COUNT' was not declared in this scope`

This means an animation array is still using `FRAME_COUNT`.

Fix this:

```cpp
const uint8_t PROGMEM sleepy[FRAME_COUNT][FRAME_BYTES] = {
```

Change it to:

```cpp
const uint8_t sleepy[SLEEPY_FRAME_COUNT][FRAME_BYTES] PROGMEM = {
```

### `too many initializers`

This means the frame count does not match the number of frame blocks.

Example problem:

```cpp
#define SLEEPY_FRAME_COUNT 5
```

But the array contains 13 frames.

Fix:

```cpp
#define SLEEPY_FRAME_COUNT 13
```

### The screen is blank

Possible causes:

- OLED address is wrong
- The bitmap data is all `0x00`
- The image was converted with the wrong threshold
- The image colours need to be inverted
- The OLED wiring is incorrect

A blank frame usually looks like this:

```cpp
0x00, 0x00, 0x00, 0x00
```

A visible frame should contain a mix of values, such as:

```cpp
0x00, 0x18, 0x3C, 0x7E, 0xFF
```

### BMI160 is not detected

If the I2C scanner only shows:

```text
0x3C
```

then the OLED is detected, but the BMI160 is not.

Check:

- BMI160 VIN goes to 3.3V
- BMI160 GND goes to GND
- BMI160 SDA goes to GPIO 8
- BMI160 SCL goes to GPIO 9
- BMI160 CS goes to 3.3V
- BMI160 SA0 goes to GND or 3.3V

Expected BMI160 address:

```text
0x68 or 0x69
```

### Tap does not trigger freaky

Lower the tap threshold:

```cpp
const float TAP_JERK_THRESHOLD = 0.03;
```

Also check Serial Monitor at `115200` and watch the printed jerk value.

### Freaky triggers too often

Raise the tap threshold:

```cpp
const float TAP_JERK_THRESHOLD = 0.08;
```

Or increase the cooldown:

```cpp
const unsigned long TRIGGER_COOLDOWN = 1000;
```

## Recommended Animation Settings

For smooth playback:

```text
Resolution: 128x64
Colour mode: 1-bit monochrome
Frame count: 5 to 30 frames per animation
Frame delay: 60 to 120 ms
Background: black
Drawing colour: white
```

## Recommended Current Tuning

Current working direction:

```cpp
const unsigned long MOTION_READ_DELAY = 40;
const unsigned long FREAKY_DURATION = 3000;
const unsigned long TRIGGER_COOLDOWN = 500;

const float TAP_JERK_THRESHOLD = 0.05;
const float SHAKE_G_THRESHOLD = 0.55;
```

If the tap feels too hard to trigger:

```cpp
const float TAP_JERK_THRESHOLD = 0.03;
```

If the tap triggers accidentally:

```cpp
const float TAP_JERK_THRESHOLD = 0.08;
```

## Git Usage

After making changes:

```bash
git add .
git commit -m "Update OLED motion animation logic"
git push
```

If GitHub rejects the push with `fetch first`, pull the remote changes first:

```bash
git pull --rebase origin main
git push
```

To check the repo status:

```bash
git status
```

To check the connected GitHub remote:

```bash
git remote -v
```

## Future Improvements

Possible upgrades:

- Add a dedicated idle animation
- Add a happy animation
- Add a blink animation
- Add an angry animation
- Add a proper overstimulated animation later
- Trigger different animations based on different gesture types
- Use a button or touch sensor for cleaner interaction
- Add sound effects with a buzzer
- Store each animation in separate header files
- Build a small tool to convert Procreate GIFs directly into `frames.h`
- Add a battery and enclosure for a finished desk buddy

## Credits

Created as a custom ESP32-C3 OLED animation desk buddy using frame data generated from Procreate artwork and motion input from a BMI160 sensor.
