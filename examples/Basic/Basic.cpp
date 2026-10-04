/*
 * chrDisp example sketch
 *
 * This example shows how to use the chrDisp class for robust ESP display
 * setup, event-driven status monitoring, and hardware-agnostic rendering
 * using Adafruit_GFX.
 *
 * It is useful for devices that need:
 *   - display management with auto-sleep and dimming features
 *   - serial event logging for debugging and state changes
 *   - hardware-agnostic callbacks for drawing, clearing, and power management
 *   - non-blocking, phase-based blinking of UI elements and status icons
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "chrDisp.h"

// Kijelző paraméterek
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Global GFX object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// create chrDisp object:
// 15000 ms auto-sleep time, room for 5 display items, max 10 characters per text,
// no skipChars storage, foreground: 1 (white), background: 0 (black), ID: 1
chrDisp myDisp(&display, 15000, 5, 10, 0, 1, 0, 1);

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

    // Initialize I2C display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }

    // Set hardware-specific callbacks
    // These connect the hardware-agnostic class to the specific GFX implementation
    myDisp.setHwUpdateCallback([]() { display.display(); });
    myDisp.setHwClearCallback([]()  { display.clearDisplay(); });
    myDisp.setHwDimCallback([](bool dim) { display.dim(dim); });
    myDisp.setHwInvertCallback([](bool inv) { display.invertDisplay(inv); });

    // Register event handler
    myDisp.setEventCallback(dispEventHandler);

    // Define display items. chrDisp copies them into its own storage,
    // so the local variables below can safely go out of scope.
    chrDisp::DisplayItem item;

    // 0. Simple text in the top-left corner (copied, truncated to the max text length)
    char label[] = "chrDisp OK";
    item.text = label;
    item.size = 1;
    item.x = 0;
    item.y = 0;
    item.type = chrDisp::ITEM_TEXT;
    myDisp.setDisplayItem(0, item);

    // From here on the items are icons: no text, size = height in pixels
    item = chrDisp::DisplayItem();

    // 1. Battery icon in the top-right corner (75% charged)
    item.type = chrDisp::ITEM_BATTERY;
    item.size = 10;
    item.width = 20;
    item.x = 105;
    item.y = 0;
    item.data = 75;
    myDisp.setDisplayItem(1, item);

    // 2. WiFi icon, blinking (0b10101010 pattern) with 50% signal strength
    item.type = chrDisp::ITEM_WIFI;
    item.size = 16;
    item.width = 16;
    item.x = 0;
    item.y = 20;
    item.blink = 0b10101010;
    item.data = 50;
    myDisp.setDisplayItem(2, item);

    // 3. Potmeter icon (circle), size = 0-degree starting point, value 128 (half)
    item = chrDisp::DisplayItem();
    item.type = chrDisp::ITEM_POTMETER;
    item.size = 0;
    item.width = 24;
    item.x = 30;
    item.y = 20;
    item.data = 128;
    myDisp.setDisplayItem(3, item);

    // 4. Disk icon with 90% fill level
    item = chrDisp::DisplayItem();
    item.type = chrDisp::ITEM_DISK;
    item.size = 16;
    item.width = 16;
    item.x = 70;
    item.y = 20;
    item.data = 90;
    myDisp.setDisplayItem(4, item);

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

    // Call main logic loop; stored items are redrawn only when they changed or blink
    // Parameters: (wakeUp, invert_display, clear_screen_before)
    myDisp.loop(userActivityDetected, false, false);

    delay(20);
}