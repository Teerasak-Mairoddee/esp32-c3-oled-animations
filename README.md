# ESP32-C3 OLED Animation Project

This project displays custom frame-by-frame animations on an SSD1306 OLED screen using an ESP32-C3.

The animation frames are exported from Procreate, converted into 1-bit monochrome bitmap data, stored in `frames.h`, and played on the OLED using the Adafruit SSD1306 library.

## Project Overview

The project is designed to play multiple OLED animations and cycle between them automatically.

Example animations:

- `freaky`
- `sleepy`

Each animation is stored as a separate frame bank inside `frames.h`. The main Arduino sketch reads those frame banks and displays them one frame at a time.

## Hardware Used

- ESP32-C3 development board
- SSD1306 OLED display
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
- Reading animation frame data
- Drawing frames to the screen
- Controlling frame speed
- Looping animations
- Switching between different animations

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

You also need ESP32 board support installed in Arduino IDE.

## Board Selection

In Arduino IDE, select the correct ESP32-C3 board.

The exact board depends on your module, but common options include:

- ESP32C3 Dev Module
- Seeed XIAO ESP32C3
- LOLIN C3 Mini

Choose the board that matches your hardware.

## Basic Wiring

Typical I2C OLED wiring:

| OLED Pin | ESP32-C3 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | SDA pin |
| SCL | SCL pin |

The exact SDA and SCL pins depend on your ESP32-C3 board.

## How The Animation System Works

The main sketch uses this structure:

```cpp
struct Animation {
  const uint8_t (*frames)[FRAME_BYTES];
  uint16_t frameCount;
  uint16_t frameDelay;
  uint8_t loopsBeforeNext;
};
```

Each animation is added to this list:

```cpp
const Animation animations[] = {
  { freaky,  FREAKY_FRAME_COUNT, 100, 5 },
  { sleepy,  SLEEPY_FRAME_COUNT, 80,  2 },
};
```

The values mean:

```text
animation frame bank, number of frames, delay per frame, loops before switching
```

Example:

```cpp
{ freaky, FREAKY_FRAME_COUNT, 100, 5 }
```

This means:

- Play the `freaky` animation
- Use the number of frames defined by `FREAKY_FRAME_COUNT`
- Wait 100 ms between frames
- Loop it 5 times before moving to the next animation

## How To Add A New Animation

### 1. Convert your animation frames

Create or export your animation from Procreate.

Recommended export process:

```text
Procreate animation
→ GIF or PNG sequence
→ 128x64 black and white frames
→ C bitmap array
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

```cpp
const Animation animations[] = {
  { freaky, FREAKY_FRAME_COUNT, 100, 5 },
  { sleepy, SLEEPY_FRAME_COUNT, 80, 2 },
  { happy,  HAPPY_FRAME_COUNT,  90, 3 },
};
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

## Git Usage

After making changes:

```bash
git add .
git commit -m "Update OLED animations"
git push
```

## Future Improvements

Possible upgrades:

- Trigger animations with buttons
- Use random animation selection
- Add idle, blink, sleep, angry, and happy states
- Add sound effects with a buzzer
- Add different animations based on sensors
- Store each animation in separate header files
- Build a small tool to convert Procreate GIFs directly into `frames.h`

## Credits

Created as a custom ESP32-C3 OLED animation project using frame data generated from Procreate artwork.
