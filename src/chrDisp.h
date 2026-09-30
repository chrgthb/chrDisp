#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <functional>

class chrDisp {
public:
    using EventCallback = std::function<void(int8_t, const char*)>;

    // Event codes
    enum EventCode {
        EVENT_ERR = -10,
        EVENT_WARN = -1,
        EVENT_OK = 0,
        EVENT_NOTICE = 10,
        EVENT_ON = 12,
        EVENT_OFF = 14,
        EVENT_REFRESHED = 16
    };

    // Types of displayable items
    enum ItemTypes {
        ITEM_TEXT = -1,
        ITEM_VBAR,
        ITEM_HBAR,
        ITEM_BATTERY,
        ITEM_CHARGE,
        ITEM_CROSSHAIR,
        ITEM_DISK,
        ITEM_WIFI,
        ITEM_AP,
        ITEM_POTMETER,
        ITEM_COUNT
    };

    // Properties of displayable items
    struct DisplayItem {
        const char* text = nullptr; // Text to display (not relevant for icons)
        uint8_t size = 2;           // Font size for text, height in pixels for icons
        uint8_t width = 0;          // For icons
                                    // - if set for text and text is smaller then given width, text will be centered within the width
        int16_t x = 0;
        int16_t y = 0;
        ItemTypes type = ITEM_TEXT;
        int16_t invert = 0;         // 0 means no inversion
                                    // - for icons all other values indicate inversion
                                    // - for texts -1 means total inversion,
                                    // - positive numbers indicate inversion of the character itself (1st, 2nd, etc.),
                                    // - negative numbers indicate inversion of every 2nd, 3rd, 4th character respectively
        uint8_t blink = 0b1111;     // In which quarter of the blinking the item should appear on the display
        int16_t blinkChar = 0;      // For texts: blink characters with the same method like inversion works for texts (but 0 and -1 means the same, a fully blinking text)
        const char* skipChars = nullptr; // Characters to ignore during blinking or inversion (you can set for eg. space or new line chars if they just separate text parts)
        uint8_t frame = 0;          // A button like rounded rect around the item
                                    // - numbers mean the thickness of the frame
                                    // - if color off the frame equals to background, it will be a rounded rectangle with foreground color
                                    // - else it will be a filled rounded rectangle
                                    // - FRAMES WON'T BLINK automatically
        uint8_t data = 0;           // Percent or level for icons
    };

    // Constructor: expects a GFX pointer
    chrDisp(Adafruit_GFX* gfx, uint32_t turnOffMs = 10000, uint16_t fgColor = 0xFFFF, uint16_t bgColor = 0x0000, uint8_t id = 0);

    // Types of hardware actions
    using HardwareAction = std::function<void()>;
    using HardwareBoolAction = std::function<void(bool)>;

    // Setter methods for hardware-specific functions
    void setHwUpdateCallback(HardwareAction cb) { _hwUpdate = cb; }     // e.g., call display()
    void setHwClearCallback(HardwareAction cb) { _hwClear = cb; }       // e.g., call clearDisplay()
    void setHwPowerCallback(HardwareBoolAction cb) { _hwPower = cb; }   // e.g., ON/OFF commands
    void setHwDimCallback(HardwareBoolAction cb) { _hwDimming = cb; }   // e.g., dim() command
    void setHwInvertCallback(HardwareBoolAction cb) { _hwInvert = cb; } // e.g., invertDisplay()

    void setColors(uint16_t fg, uint16_t bg);
    void off();
    void on();
    void dim(bool dim_on = true);
    void clear();

    // In every loop() (automatically handles on / off)
    // - return value is true if there was a write to the display
    // - allowStoreForAutoBlink, if true: if items is not nullptr, the pointer and size of items are stored (the memory content must be accessible!), in this case, during blinking, there is no need to resend the items, the function itself handles it
    bool loop(bool turnOnResetSleep, bool allowStoreForAutoBlink, const DisplayItem* items, uint8_t itemCount = 0, bool invertDisplay = false, bool clearDisplay = true);
    bool loop(bool turnOnResetSleep, const char* text, uint8_t size = 2, bool invertDisplay = false, bool clearDisplay = true);
    bool loop(bool turnOnResetSleep);   // loop(true, nullptr); in this case the compiler would not know which loop() method to call, so this can also be used or the type must be specified, e.g   .: (const MyDisplayTextItem*)nullptr

    void setEventCallback(EventCallback cb);
    void setTurnOff(uint32_t turnOffMs);
    bool isOn();
    uint32_t getTurnOffMs();
    
    void drawVBar     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawHBar     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawBattery  (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawCharge   (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t notUsed = 0);
    void drawCrosshair(int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t notUsed = 0);
    void drawDisk     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawWiFi     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawAP       (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t notUsed = 0);
    void drawPotmeter (int16_t x, int16_t y, uint16_t width, uint16_t zeroPointDeg, uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t level = 0);

    // Defining the pointer
    using DrawFuncPtr = void (chrDisp::*)(int16_t, int16_t, uint16_t, uint16_t, uint8_t, bool, uint8_t, uint8_t);

    // Creating an array of pointers (must match the enum)
    static constexpr DrawFuncPtr DRAW_FUNCTIONS[9] = {
        &chrDisp::drawVBar,      // MY_DISPLAY_VBAR (0)
        &chrDisp::drawHBar,      // MY_DISPLAY_HBAR (1)
        &chrDisp::drawBattery,   // MY_DISPLAY_BATTERY (2)
        &chrDisp::drawCharge,    // MY_DISPLAY_CHARGE (3)
        &chrDisp::drawCrosshair, // MY_DISPLAY_CROSSHAIR (4)
        &chrDisp::drawDisk,      // MY_DISPLAY_DISK (5)
        &chrDisp::drawWiFi,      // MY_DISPLAY_WIFI (6)
        &chrDisp::drawAP,        // MY_DISPLAY_AP (7)
        &chrDisp::drawPotmeter   // MY_DISPLAY_POTMETER (8)
    };

    // Creating the calling function
    void drawItem(ItemTypes type, int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t blink = 0b1111, bool invert = false, uint8_t frame = 0, uint8_t val = 0) {
        if (type >= 0 && type < ITEM_COUNT) {
            // Calling the pointer on the current (this) object
            (this->*DRAW_FUNCTIONS[type])(x, y, w, h, blink, invert, frame, val);
        }
    }


private:
    Adafruit_GFX* _gfx;
    
    // Hardware callbacks
    HardwareAction _hwUpdate = nullptr;
    HardwareAction _hwClear = nullptr;
    HardwareBoolAction _hwPower = nullptr;
    HardwareBoolAction _hwDimming = nullptr;
    HardwareBoolAction _hwInvert = nullptr;

    uint8_t _id;
    uint16_t _width;
    uint16_t _height;
    uint16_t _fgColor;
    uint16_t _bgColor;
    uint32_t _turnOffMs;
    bool _isOn;
    bool _isDimmed;
    bool _needRedraw;
    uint32_t _lastRedrawMillis;
    uint32_t _lastOnMillis;
    EventCallback _onEvent;
    uint8_t _blinkCounter;  // blinking enabled element
    uint32_t _lastBlinkCounterChangeMillis;
    const DisplayItem* _lastAutoBlinkItems;
    uint8_t _lastAutoBlinkItemsCount;
    bool _firstLoop;
    
    static constexpr uint16_t BLINK_INTERVAL_MS = 250;
    static constexpr uint16_t DIM_TIME_MS = 5000;
    static constexpr uint16_t MIN_TURN_OFF_MS = DIM_TIME_MS * 2;
    static constexpr uint8_t ROUNDED_RECT_RADIUS = 3;   // 3 pixel won't be a problem on filled icons

    bool _checkBlinkBit(uint8_t blink, int8_t at_index);
    void _drawFrameHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert = false, uint8_t frame = 0);
    void _drawVFilledRectHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert = false, uint8_t percent = 0, int16_t outlineRadius = 0, bool batteryTip = false);
    void _drawHFilledRectHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert = false, uint8_t percent = 0, int16_t outlineRadius = 0);
    void _drawTextHelper(int16_t x, int16_t y, uint8_t size, const char* text, uint8_t width = 0, uint8_t blink = 0b1111, int16_t blinkChar = 0, int16_t invert = 0, const char* skipChars = nullptr, uint8_t frame = 0);
    // Internal event handler
    void _fireEvent(int8_t code, const char* action);
};
