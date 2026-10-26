/*
 * chrDisp example sketch
 *
 * This example shows how to use the chrDisp class for robust ESP display
 * setup, event-driven status monitoring, and hardware-agnostic rendering
 * using Adafruit_GFX, on a 128x128 SSD1327 SPI grayscale (4-bit, 16 levels) OLED.
 *
 * The display-wide fgColor / bgColor are the defaults of every item; an item can
 * override them with its own fgColor / bgColor (ItemProps). With a monochrome
 * (SSD1306) or an RGB565 color display the same code works, only the color values differ.
 *
 * It is useful for devices that need:
 *   - display management with auto-sleep and dimming features
 *   - serial event logging for debugging and state changes
 *   - hardware-agnostic callbacks for drawing, clearing, and power management
 *   - non-blocking, phase-based blinking of UI elements and status icons
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>
#include "chrDisp.h"

// Display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128

// SPI pins (ESP32 VSPI: SCK = 18, MOSI = 23)
#define OLED_CS 5
#define OLED_DC 16
#define OLED_RESET 17

// Contrast levels (0..255) for the dimming callback
#define CONTRAST_NORMAL 0x7F
#define CONTRAST_DIMMED 0x10

// Gray levels of a 4-bit display: 0 (black) ... 15 (white)
#define GRAY_DARK 3
#define GRAY_MID 8
#define GRAY_LIGHT 12

// Global GFX object
Adafruit_SSD1327 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// create chrDisp object:
// 15000 ms auto-sleep time, room for 6 display items, max 12 characters per text,
// no skipChars storage, default foreground: 15 (white), default background: 0 (black), ID: 1
chrDisp myDisp(&display, 15000, 6, 12, 0, SSD1327_WHITE, SSD1327_BLACK, 1);

// Event handler callback function
void dispEventHandler(int8_t code, uint8_t displayId) {
    // Log events to the serial port
    Serial.printf("[chrDisp Event] Display: %u | Code: %d | Event: %s\n",
                  static_cast<unsigned>(displayId), code,
                  myDisp.eventName(static_cast<chrDisp::EventCode>(code)));
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting chrDisp example...");

    // Initialize SPI display
    if (!display.begin()) {
        Serial.println(F("SSD1327 allocation failed"));
        for (;;);
    }

    // Set hardware-specific callbacks
    // These connect the hardware-agnostic class to the specific GFX implementation
    myDisp.setHwUpdateCallback([]() { display.display(); });
    myDisp.setHwClearCallback([]()  { display.clearDisplay(); });
    myDisp.setHwPowerCallback([](bool on) { display.oled_command(on ? SSD1327_DISPLAYON : SSD1327_DISPLAYOFF); });
    myDisp.setHwDimCallback([](bool dim) { display.setContrast(dim ? CONTRAST_DIMMED : CONTRAST_NORMAL); });
    myDisp.setHwInvertCallback([](bool inv) { display.invertDisplay(inv); });

    // Register event handler
    myDisp.setEventCallback(dispEventHandler);

    // Define display items. chrDisp copies them into its own storage,
    // so the local variables below can safely go out of scope.
    chrDisp::ConstDisplayItem item;

    // 0. Right-aligned text within an 80-pixel area and vertically centered in a 16-pixel-high frame.
    //    The item has its own colors: white text on a dark gray button (the frame is filled with bgColor)
    item.text = "chrDisp OK";
    item.x = 0;
    item.y = 0;
    item.width = -80;
    item.height = 16;
    item.frame = 1;
    item.type = chrDisp::ITEM_TEXT;
    item.fgColor = SSD1327_WHITE;
    item.bgColor = GRAY_DARK;
    myDisp.setDisplayItem(0, item);

    // From here on the items are icons: width and height are their pixel dimensions
    item = chrDisp::ConstDisplayItem();

    // 1. Battery icon in the top-right corner (75% charged)
    item.type = chrDisp::ITEM_BATTERY;
    item.width = 20;
    item.height = 10;
    item.x = 105;
    item.y = 0;
    item.data = 75;
    myDisp.setDisplayItem(1, item);     // default colors (white on black)

    // 2. WiFi icon, blinking (0b10101010 pattern) with 50% signal strength
    item.type = chrDisp::ITEM_WIFI;
    item.width = 16;
    item.height = 16;
    item.x = 0;
    item.y = 20;
    item.blink = 0b10101010;
    item.data = 50;
    item.fgColor = GRAY_LIGHT;          // only the foreground differs, the background stays default
    myDisp.setDisplayItem(2, item);

    // 3. Potmeter icon (circle), zero-point angle = 0 degrees, value 128 (half)
    item = chrDisp::ConstDisplayItem();
    item.type = chrDisp::ITEM_POTMETER;
    item.width = 24;
    item.zeroPointDeg = 0;
    item.x = 30;
    item.y = 20;
    item.data = 128;
    item.fgColor = SSD1327_WHITE;
    item.bgColor = GRAY_DARK;
    myDisp.setDisplayItem(3, item);

    // 4. Disk icon with 90% fill level
    item = chrDisp::ConstDisplayItem();
    item.type = chrDisp::ITEM_DISK;
    item.width = 16;
    item.height = 16;
    item.x = 70;
    item.y = 20;
    item.data = 90;
    item.fgColor = GRAY_MID;
    myDisp.setDisplayItem(4, item);

    // 5. Fully inverted text: invert swaps the item's fgColor and bgColor,
    //    so this is black text on a light gray background (works with any color depth)
    item = chrDisp::ConstDisplayItem();
    item.text = "INVERTED";
    item.x = 0;
    item.y = 50;
    item.invert = -1;
    item.fgColor = SSD1327_WHITE;
    item.bgColor = GRAY_LIGHT;
    myDisp.setDisplayItem(5, item);

    // Turn on the display (this also generates an EVENT_ON event)
    myDisp.on();
}

void loop() {
    static uint32_t lastActivity = 0;
    bool userActivityDetected = false;

    // Simulate a button press or data refresh every 20 seconds.
    // This keeps the display awake or wakes it up after the 15s auto-sleep.
    if (millis() - lastActivity > 20000) {
        userActivityDetected = true;
        lastActivity = millis();
        Serial.println("Simulated user activity (wakeup)!");
    }

    // Every 3 seconds update the battery icon in place (no item copy, auto-marked for redraw)
    static uint32_t lastEdit = 0;
    if (millis() - lastEdit > 3000) {
        lastEdit = millis();
        if (chrDisp::DisplayItem* bat = myDisp.editDisplayItem(1)) {
            bat->data = (bat->data >= 100) ? 0 : bat->data + 25;
        }
    }

    // Call main logic loop; stored items are redrawn only when they changed or blink
    // Parameters: (wakeUp, invert_display (hardware inversion callback), clear_screen_before)
    myDisp.loop(userActivityDetected, false, false);

    delay(20);
}