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
- Character-level and item-level blinking (using 8-bit phase bitmasks) and inversion options
- Efficient item storage: items are copied into library-owned buffers and redrawn only when changed or blinking
- Programmer-defined limits (max items, max text length, max skipChars length) allocated once in the constructor, no dangling pointers

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
// 10 s auto turn-off, room for 2 items, max 12 characters per text
chrDisp myDisp(&display, 10000, 2, 12);

void onDispEvent(int8_t code, uint8_t displayId) {
	Serial.printf("display=%u event=%s (%d)\n",
	              static_cast<unsigned>(displayId),
	              myDisp.eventName(static_cast<chrDisp::EventCode>(code)),
	              code);
}

void hwUpdate() {
	display.display();
}

void setup() {
	Serial.begin(115200);
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	myDisp.setEventCallback(onDispEvent);
	myDisp.setHwUpdateCallback(hwUpdate);

	chrDisp::DisplayItem item;
	char text[] = "Hello World!";
	item.text = text; // copied into the library's own buffer
	myDisp.setDisplayItem(0, item);

	myDisp.on();
}

void loop() {
	bool userActivity = false; // Set to true on button press or user interaction
	myDisp.loop(userActivity, false, false);
}
```

## API Reference (chrDisp Class)

Public methods declared in [src/chrDisp.h](src/chrDisp.h):

- `chrDisp(Adafruit_GFX* gfx, uint32_t turnOffMs = 10000, uint8_t maxDisplayItems = 0, uint8_t maxTextLen = 0, uint8_t maxSkipCharsLen = 0, uint16_t fgColor = 0xFFFF, uint16_t bgColor = 0x0000, uint8_t id = 0)`
	- Constructor initializing display pointer, sleep timeout, the maximum number of display items, the maximum text / skipChars length per item (buffers are allocated once here), foreground/background colors, and display instance ID. The object is not copyable.
- `bool setDisplayItem(uint8_t index, const DisplayItem& item)`
	- Copies the item (including text and skipChars, truncated to the configured maximums) into slot `index` and marks it for redraw. Returns `false` if `index` is out of range.
- `DisplayItem* editDisplayItem(uint8_t index)`
	- Low-cost in-place modification: returns a pointer to the stored item (no copy) and marks it for redraw. Returns `nullptr` if `index` is out of range. Cleared items stay cleared; use `setDisplayItem()` to show them again. `text` and `skipChars` point to fixed buffers (`maxTextLen + 1` / `maxSkipCharsLen + 1` bytes): write them with `strlcpy()` and never reassign the pointers.
- `bool clearDisplayItem(uint8_t index)`
	- Marks the item to be erased from the screen on the next `loop()`. Returns `false` if `index` is out of range.
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
- `bool loop(bool turnOnResetSleep, bool invertDisplay = false, bool clearDisplay = true)`
	- Main execution loop; evaluates sleep/dimming timers, handles blinking phases and draws the stored display items. With `clearDisplay = true` the screen is cleared and all items are redrawn; otherwise only changed or blinking items are redrawn.
- `bool loop(bool turnOnResetSleep, const char* text, uint8_t size = 2, bool invertDisplay = false, bool clearDisplay = true)`
	- Same as above, additionally drawing a simple text at the top-left corner.
- `void setEventCallback(EventCallback cb)`
	- Registers a callback receiving the event code and display ID. The callback no longer receives a formatted message string; use `eventName()` when a readable label is needed.
- `const char* eventName(EventCode code)`
	- Returns a readable event name from the event code, without formatting or allocating a message.
- `void setBlinkCallback(BlinkCallback cb)`
	- Registers blink callback.
- `void setTurnOff(uint32_t turnOffMs)`
	- Sets auto turn-off timeout in milliseconds.
- `bool isOn()`
	- Returns `true` if display is currently powered on.
- `uint32_t getTurnOffMs()`
	- Returns configured turn-off timeout in milliseconds.
- Icon drawing methods:
	- `drawVBar(...)`, `drawHBar(...)`, `drawBattery(...)`, `drawCharge(...)`, `drawCrosshair(...)`, `drawDisk(...)`, `drawWiFi(...)`, `drawAP(...)`, `drawPotmeter(...)`
- `void drawItem(ItemTypes type, int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t val = 0)`
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

Items are set with `setDisplayItem()` using the `DisplayItem` struct (the `text` and `skipChars` pointers are only read during the call):

```cpp
struct DisplayItem {
    char* text = nullptr;
    uint8_t size = 2;
    uint16_t width = 0;
    int16_t x = 0;
    int16_t y = 0;
    ItemTypes type = ITEM_TEXT;
    int16_t invert = 0;
    uint8_t blink = 0b11111111;
    int16_t blinkChar = 0;
    char* skipChars = nullptr;
    uint8_t frame = 0;
    uint8_t data = 0;
    ItemStates state = ITEM_CLEARED; // managed by the library
};
```

Existing items can be changed cheaply with `editDisplayItem()`:

```cpp
if (chrDisp::DisplayItem* it = myDisp.editDisplayItem(0)) {
    strlcpy(it->text, "Updated", 13); // 12 = maxTextLen in the Quick Start
    it->blink = 0b11110000;
}
```

## Event Codes

`chrDisp::EventCode` values:

- `EVENT_ERR = -10`
- `EVENT_WARN = -5`
- `EVENT_OK = 0`
- `EVENT_NOTICE = 10`
- `EVENT_DIM = 12`
- `EVENT_INVERT = 14`
- `EVENT_ON = 20`
- `EVENT_OFF = 25`
- `EVENT_REFRESHED = 30`

Use `eventName(EventCode)` to map an event code to a static, human-readable name without constructing or copying a formatted message. The event callback receives `(int8_t code, uint8_t displayId)`.

## Build

This repository is configured with PlatformIO.

From project root:

```bash
platformio run
```

## Notes

- Blinking logic uses a 250 ms tick interval (`BLINK_INTERVAL_MS = 250`) and a 8-bit bitmask (`0b11111111` = always visible).
- Automatic dimming activates when 5 seconds (`DIM_TIME_MS = 5000`) remain before complete turn-off.
- The minimum allowed turn-off timeout is 10 seconds (`MIN_TURN_OFF_MS = 10000`).

## License

This project is licensed under the MIT License.

You are free to download, use, modify, and distribute this code, including for commercial use, as long as the MIT license notice is kept with substantial portions of the software.

See [LICENSE](LICENSE) for the full text.