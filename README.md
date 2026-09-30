# chrDisp

`chrDisp` is a small Arduino library wrapper around `Adafruit_GFX` to provide a practical display manager for ESP devices and other microcontrollers.

It is designed for projects that need:

- Automatic display sleep / turn-off and dimming timer management
- Hardware-agnostic hooks for power control, backlight dimming, display clearing, and screen refreshing
- Built-in customizable UI icons and graphical indicators (Battery, WiFi, AP, Progress Bars, Disk, Potentiometer, Crosshair, Charge)
- Non-blocking loop processing with automatic 4-phase blinking bitmasks and text inversion effects
- Event-driven display status reporting

The library targets the Arduino framework and works with any display driver compatible with `Adafruit_GFX` (e.g., SSD1306, ST7789, ILI9341, etc.).

## Features

- Non-blocking display manager loop (`chrDisp::loop()`)
- Automatic sleep and dimming management based on configurable inactivity timers
- Flexible hardware callbacks for display driver operations (`hwUpdate`, `hwClear`, `hwPower`, `hwDimming`, `hwInvert`)
- Rich set of built-in UI elements and vector icons
- Character-level and item-level blinking (using 4-bit phase bitmasks) and inversion options
- Efficient auto-blinking storage to handle blinking phases without redundant redrawing overhead in the main loop

## Dependency

This library depends on:

- `adafruit/Adafruit GFX Library`

Defined in [library.json](library.json) and [platformio.ini](platformio.ini).

## Quick Start

Example initialization flow with an SSD1306 OLED display:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "chrDisp.h"

Adafruit_SSD1306 display(128, 64, &Wire, -1);
chrDisp myDisp(&display, 10000); // 10 second auto turn-off timeout

void onDispEvent(int8_t code, const char* msg) {
	Serial.printf("event=%d msg=%s\n", code, msg ? msg : "null");
}

void hwUpdate() {
	display.display();
}

void setup() {
	Serial.begin(115200);
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	myDisp.setEventCallback(onDispEvent);
	myDisp.setHwUpdateCallback(hwUpdate);

	myDisp.on();
}

void loop() {
	bool userActivity = false; // Set to true on button press or user interaction
	myDisp.loop(userActivity, "Hello World!", 2, false, true);
}
```

## API Reference (chrDisp Class)

Public methods declared in [chrDisp.h](chrDisp.h):

- `chrDisp(Adafruit_GFX* gfx, uint32_t turnOffMs = 10000, uint16_t fgColor = 0xFFFF, uint16_t bgColor = 0x0000, uint8_t id = 0)`
	- Constructor initializing display pointer, sleep timeout, foreground/background colors, and display instance ID.
- `void setHwUpdateCallback(HardwareAction cb)`
	- Registers callback for updating/flushing the display hardware (e.g., `display.display()`).
- `void setHwClearCallback(HardwareAction cb)`
	- Registers callback for clearing the screen buffer.
- `void setHwPowerCallback(HardwareBoolAction cb)`
	- Registers callback for powering display hardware ON/OFF.
- `void setHwDimCallback(HardwareBoolAction cb)`
	- Registers callback for hardware backlight dimming ON/OFF.
- `void setHwInvertCallback(HardwareBoolAction cb)`
	- Registers callback for hardware screen inversion ON/OFF.
- `void setColors(uint16_t fg, uint16_t bg)`
	- Updates foreground and background colors.
- `void on()`
	- Powers on the display and resets sleep timers.
- `void off()`
	- Powers off the display manually.
- `void dim(bool dim_on = true)`
	- Controls dimming state manually.
- `void clear()`
	- Clears display buffer using hardware callback or fills with background color as fallback.
- `bool loop(bool turnOnResetSleep, bool allowStoreForAutoBlink, const DisplayItem* items, uint8_t itemCount = 0, bool invertDisplay = false, bool clearDisplay = true)`
	- Main execution loop; renders display items, evaluates sleep/dimming timers, and handles blinking phases.
- `bool loop(bool turnOnResetSleep, const char* text, uint8_t size = 2, bool invertDisplay = false, bool clearDisplay = true)`
	- Overloaded loop helper for rendering simple text strings.
- `bool loop(bool turnOnResetSleep)`
	- Overloaded loop helper to refresh display state using previously stored auto-blink items.
- `void setEventCallback(EventCallback cb)`
	- Registers global event callback.
- `void setTurnOff(uint32_t turnOffMs)`
	- Sets auto turn-off timeout in milliseconds.
- `bool isOn()`
	- Returns `true` if display is currently powered on.
- `uint32_t getTurnOffMs()`
	- Returns configured turn-off timeout in milliseconds.
- Icon drawing methods:
	- `drawVBar(...)`, `drawHBar(...)`, `drawBattery(...)`, `drawCharge(...)`, `drawCrosshair(...)`, `drawDisk(...)`, `drawWiFi(...)`, `drawAP(...)`, `drawPotmeter(...)`
- `void drawItem(ItemTypes type, int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t val = 0)`
	- Generic dispatcher for drawing any item type by enum index.

## Display Items and UI Elements

Supported `ItemTypes`:

- `ITEM_TEXT = -1`
- `ITEM_VBAR = 0`
- `ITEM_HBAR = 1`
- `ITEM_BATTERY = 2`
- `ITEM_CHARGE = 3`
- `ITEM_CROSSHAIR = 4`
- `ITEM_DISK = 5`
- `ITEM_WIFI = 6`
- `ITEM_AP = 7`
- `ITEM_POTMETER = 8`

Items are passed using the `DisplayItem` struct:

```cpp
struct DisplayItem {
    const char* text = nullptr;
    uint8_t size = 2;
    uint8_t width = 0;
    int16_t x = 0;
    int16_t y = 0;
    ItemTypes type = ITEM_TEXT;
    int16_t invert = 0;
    uint8_t blink = 0b1111;
    int16_t blinkChar = 0;
    const char* skipChars = nullptr;
    uint8_t frame = 0;
    uint8_t data = 0;
};
```

## Event Codes

`chrDisp::EventCode` values:

- `EVENT_ERR = -10`
- `EVENT_WARN = -1`
- `EVENT_OK = 0`
- `EVENT_NOTICE = 10`
- `EVENT_ON = 12`
- `EVENT_OFF = 14`
- `EVENT_REFRESHED = 16`

Typical event log messages include:

- `disp [0] turned on`
- `disp [0] turned off`
- `disp [0] dim on`
- `disp [0] dim off`
- `disp [0] refreshed`
- `disp [0] GFX obj not initialized`

## Build

This repository is configured with PlatformIO.

From project root:

```bash
platformio run
```

## Notes

- Blinking logic uses a 250 ms tick interval (`BLINK_INTERVAL_MS = 250`) and a 4-bit bitmask (`0b1111` = always visible).
- Automatic dimming activates when 5 seconds (`DIM_TIME_MS = 5000`) remain before complete turn-off.
- The minimum allowed turn-off timeout is 10 seconds (`MIN_TURN_OFF_MS = 10000`).

## License

This project is licensed under the MIT License.

You are free to download, use, modify, and distribute this code, including for commercial use, as long as the MIT license notice is kept with substantial portions of the software.

See [LICENSE](LICENSE) for the full text.