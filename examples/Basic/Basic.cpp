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
// 15000 ms auto-sleep time, foreground: 1 (white), background: 0 (black), ID: 1
chrDisp myDisp(&display, 15000, 1, 0, 1);

// Event handler callback function
void dispEventHandler(int8_t code, const char* msg) {
    // Log events to the serial port
    Serial.printf("[chrDisp Event] Code: %d | Msg: %s\n", code, msg);
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

    // Define display items (DisplayItem)
    // Structure layout: {text, size(h), width(w), x, y, type, invert, blink, percent/level}
    static chrDisp::DisplayItem uiItems[] = {
        // 1. Simple text in the top-left corner
        {"chrDisp OK", 1, 0, 0, 0, chrDisp::ITEM_TEXT, 0, 0b1111, 0, nullptr, 0, 0},
        
        // 2. Battery icon in the top-right corner (75% charged)
        {nullptr, 10, 20, 105, 0, chrDisp::ITEM_BATTERY, 0, 0b1111, 0, nullptr, 0, 75},
        
        // 3. WiFi icon, blinking (0b1010 pattern) with 50% signal strength
        {nullptr, 16, 16, 0, 20, chrDisp::ITEM_WIFI, 0, 0b1010, 0, nullptr, 0, 50},
        
        // 4. Potmeter icon (circle), size = 0-degree starting point, with a 128 (half-full) value
        {nullptr, 0, 24, 30, 20, chrDisp::ITEM_POTMETER, 0, 0b1111, 0, nullptr, 0, 128},
        
        // 5. Disk icon with 90% fill level
        {nullptr, 16, 16, 70, 20, chrDisp::ITEM_DISK, 0, 0b1111, 0, nullptr, 0, 90}
    };

    // Call main logic loop
    // Parameters: (wakeUp, autoBlinkStorage, items_array, item_count, invert_base, clear_screen_before)
    myDisp.loop(userActivityDetected, true, uiItems, 5, false, true);

    delay(20);
}