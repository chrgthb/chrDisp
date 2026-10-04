#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <functional>

class chrDisp {
public:
    using EventCallback = std::function<void(int8_t, uint8_t)>; // Event callback: parameters are event code and display ID
    using BlinkCallback = std::function<void(uint8_t, uint8_t)>; // Blink callback: parameters are display ID and blink quarter

    // Event codes
    enum EventCode {
        EVENT_ERR = -10,
        EVENT_WARN = -5,
        EVENT_OK = 0,
        EVENT_NOTICE = 10,
        EVENT_DIM = 12,
        EVENT_INVERT = 14,
        EVENT_ON = 20,
        EVENT_OFF = 25,
        EVENT_REFRESHED = 30
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

    enum ItemStates {
        ITEM_TO_CLEAR = -2,         // (set by clearDisplayItem) Item needs to be cleared from the screen
        ITEM_CLEARED = -1,          // (set by clearDisplayItem) Item has been cleared from the screen
        ITEM_NORMAL = 0,            // (loop will check it) Item doesn't changed, need only redraw if it is blinking
        ITEM_NEEDS_REDRAW = 1       // (set by setDisplayItem) Item needs to be redrawn
    };

    // Properties of displayable items
    // - clearDisplayItem set needRedraw to -1, so it will be cleared from the screen, set to 0 and won't be redisplayed
    struct DisplayItem {
        char* text = nullptr;       // Text to display (not relevant for icons)
        uint8_t size = 2;           // Font size for text, height in pixels for icons
        uint16_t width = 0;         // For icons
                                    // - if set for text and text is smaller then given width, text will be centered within the width
        int16_t x = 0;
        int16_t y = 0;
        ItemTypes type = ITEM_TEXT;
        int16_t invert = 0;         // 0 means no inversion
                                    // - for icons all other values indicate inversion
                                    // - for texts -1 means total inversion,
                                    // - positive numbers indicate inversion of the character itself (1st, 2nd, etc.),
                                    // - negative numbers indicate inversion of every 2nd, 3rd, 4th character respectively
        uint8_t blink = 0b11111111; // In which quarter of the blinking the item should appear on the display (max 8 quarters)
        int16_t blinkChar = 0;      // For texts: blink characters with the same method like inversion works for texts (but 0 and -1 means the same, a fully blinking text)
        char* skipChars = nullptr;  // Characters to ignore during blinking or inversion (you can set for eg. space or new line chars if they just separate text parts)
        uint8_t frame = 0;          // A button like rounded rect around the item
                                    // - numbers mean the thickness of the frame
                                    // - if color off the frame equals to background, it will be a rounded rectangle with foreground color
                                    // - else it will be a filled rounded rectangle
                                    // - FRAMES WON'T BLINK automatically
        uint8_t data = 0;           // Percent or level for icons
        ItemStates state = ITEM_CLEARED; // Current state of the item (normal, needs redraw, to clear, cleared)
    };

    // Constructor: expects a GFX pointer
    chrDisp(Adafruit_GFX* gfx, uint32_t turnOffMs = 10000, uint8_t maxDisplayItems = 0, uint8_t maxTextLen = 0, uint8_t maxSkipCharsLen = 0, uint16_t fgColor = 0xFFFF, uint16_t bgColor = 0x0000, uint8_t id = 0);
    ~chrDisp();
    chrDisp(const chrDisp&) = delete;               // Owns raw buffers
    chrDisp& operator=(const chrDisp&) = delete;

    // Types of hardware actions
    using HardwareAction = std::function<void()>;
    using HardwareBoolAction = std::function<void(bool)>;

    // Setter methods for hardware-specific functions
    void setHwUpdateCallback(HardwareAction cb) { _hwUpdate = cb; }     // e.g., call display()
    void setHwClearCallback(HardwareAction cb) { _hwClear = cb; }       // e.g., call clearDisplay()
    void setHwPowerCallback(HardwareBoolAction cb) { _hwPower = cb; }   // e.g., ON/OFF commands
    void setHwDimCallback(HardwareBoolAction cb) { _hwDimming = cb; }   // e.g., dim() command
    void setHwInvertCallback(HardwareBoolAction cb) { _hwInvert = cb; } // e.g., invertDisplay()

    bool setDisplayItem(uint8_t index, const DisplayItem& item);
    // Direct access for in-place modification (no copy); marks a visible item for redraw
    // - text/skipChars point to fixed buffers (maxTextLen/maxSkipCharsLen + 1), write them with strlcpy, don't reassign
    // - returns nullptr if the index is invalid; cleared items stay cleared (use setDisplayItem to show them again)
    DisplayItem* editDisplayItem(uint8_t index);
    bool clearDisplayItem(uint8_t index);
    void setColors(uint16_t fg, uint16_t bg);
    void off();
    void on();
    void dim(bool dim_on = true);
    bool invert(bool invert_on = true); // Returns true if the inversion state changed
    void clear();
    void resetBlinkQuarter();
    bool checkBlinkBit(uint8_t blink) { return checkBlinkBit(blink, _blinkQuarter); }
    bool checkBlinkBit(uint8_t blink, uint8_t at_quarter);

    // In every loop() (automatically handles on / off)
    // - return value is true if there was a write to the display
    bool loop(bool turnOnResetSleep, bool invertDisplay = false, bool clearDisplay = true);
    bool loop(bool turnOnResetSleep, const char* text, uint8_t size = 2, bool invertDisplay = false, bool clearDisplay = true);

    void setEventCallback(EventCallback cb);
    void setBlinkCallback(BlinkCallback cb);
    void setTurnOff(uint32_t turnOffMs);
    bool isOn();
    bool isDimmed();
    bool isInverted();
    uint32_t getTurnOffMs();
    
    void drawVBar     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawHBar     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawBattery  (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawCharge   (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t notUsed = 0);
    void drawCrosshair(int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t notUsed = 0);
    void drawDisk     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawWiFi     (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t percent = 0);
    void drawAP       (int16_t x, int16_t y, uint16_t width, uint16_t height,       uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t notUsed = 0);
    void drawPotmeter (int16_t x, int16_t y, uint16_t width, uint16_t zeroPointDeg, uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t level = 0);

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
    void drawItem(ItemTypes type, int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t blink = 0b11111111, bool invert = false, uint8_t frame = 0, uint8_t val = 0) {
        if (type >= 0 && type < ITEM_COUNT) {
            // Calling the pointer on the current (this) object
            (this->*DRAW_FUNCTIONS[type])(x, y, w, h, blink, invert, frame, val);
        }
    }

    const char* eventName(EventCode code);


private:
    Adafruit_GFX* _gfx;
    DisplayItem* _items;
    uint8_t _maxDisplayItems;
    uint8_t _maxTextLen;
    uint8_t _maxSkipCharsLen;
    
    // Hardware callbacks
    HardwareAction _hwUpdate = nullptr;
    HardwareAction _hwClear = nullptr;
    HardwareBoolAction _hwPower = nullptr;
    HardwareBoolAction _hwDimming = nullptr;
    HardwareBoolAction _hwInvert = nullptr;

    uint8_t _id;
    uint16_t _fgColor;
    uint16_t _bgColor;
    uint32_t _turnOffMs;
    bool _isOn;
    bool _isDimmed;
    bool _isInverted;
    uint32_t _lastRedrawMillis;
    uint32_t _lastOnMillis;
    EventCallback _onEvent;
    BlinkCallback _onBlink;
    uint8_t _blinkQuarter;  // counts 0-7 (1/4 - 8/4)
    uint32_t _blinkQuarterChangedMillis;
    bool _firstLoop;
    
    static constexpr uint16_t BLINK_INTERVAL_MS = 250;
    static constexpr uint16_t DIM_TIME_MS = 5000;
    static constexpr uint16_t MIN_TURN_OFF_MS = DIM_TIME_MS * 2;
    static constexpr uint8_t ROUNDED_RECT_RADIUS = 3;   // 3 pixel won't be a problem on filled icons

    void _drawFrameHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert = false, uint8_t frame = 0);
    void _drawVFilledRectHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert = false, uint8_t percent = 0, int16_t outlineRadius = 0, bool batteryTip = false);
    void _drawHFilledRectHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert = false, uint8_t percent = 0, int16_t outlineRadius = 0);
    void _drawTextHelper(int16_t x, int16_t y, uint8_t size, const char* text, uint8_t width = 0, uint8_t blink = 0b11111111, int16_t blinkChar = 0, int16_t invert = 0, const char* skipChars = nullptr, uint8_t frame = 0);
    bool _loopImpl(bool turnOnResetSleep, bool invertDisplay, bool clearDisplay, const char* text, uint8_t textSize);
    // Internal event handler
    void _fireEvent(EventCode code);
};
