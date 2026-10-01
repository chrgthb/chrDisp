#include "chrDisp.h"
#include <cmath>

chrDisp::chrDisp(Adafruit_GFX* gfx, uint32_t turnOffMs, uint16_t fgColor, uint16_t bgColor, uint8_t id) 
    : _gfx(gfx),
      _id(id),
//      _turnOffMs(turnOffMs),
      _fgColor(fgColor),
      _bgColor(bgColor),
      _isOn(false),
      _isDimmed(false),
      _isInverted(false),
      _needRedraw(false),
      _lastRedrawMillis(0),
      _lastOnMillis(0),
      _onEvent(nullptr),
      _onBlink(nullptr),
      _blinkQuarter(0),
      _blinkQuarterChangedMillis(0),
      _lastAutoBlinkItems(nullptr),
      _lastAutoBlinkItemsCount(0),
      _firstLoop(true)
{
    chrDisp::setTurnOff(turnOffMs);
    if (_gfx != nullptr) _gfx->cp437(true);
}

void chrDisp::setColors(uint16_t fg, uint16_t bg) {
    _fgColor = fg;
    _bgColor = bg;
}

void chrDisp::off() {
    if (!_isOn) return;
    
    if (_hwPower) { _hwPower(false); }
    _isOn = false;
    
    _fireEvent(EVENT_OFF, "turned off");
}

void chrDisp::on() {
    if (!_isOn) {
        if (_hwPower) { _hwPower(true); }

        _isOn = true;
        _fireEvent(EVENT_ON, "turned on");
    }
    
    dim(false);
    _lastOnMillis = millis();
}

void chrDisp::dim(bool dim_on) {
    if (_isDimmed == dim_on) return;

    if (_hwDimming) { _hwDimming(dim_on); }

    _isDimmed = dim_on;
    _fireEvent(EVENT_NOTICE, _isDimmed ? "dim on" : "dim off");
}

bool chrDisp::invert(bool invert_on) {
    if (_isInverted == invert_on) return false;

    if (_hwInvert) {
        _hwInvert(invert_on);

        _isInverted = invert_on;
        _fireEvent(EVENT_NOTICE, _isInverted ? "invert on" : "invert off");
    }

    return _isInverted == invert_on;
}

void chrDisp::clear() {
    if (_gfx == nullptr) return;
    if (_hwClear) { _hwClear(); } 
    else { _gfx->fillScreen(_bgColor); } // Safety default GFX clear
}

void chrDisp::resetBlinkQuarter() {
    _blinkQuarter = 0;
    _blinkQuarterChangedMillis = millis();
}

bool chrDisp::checkBlinkBit(uint8_t blink, uint8_t at_quarter) {
    at_quarter = at_quarter % 8;
    return ((blink >> (7 - at_quarter)) & 1) != 0;
}

void chrDisp::setEventCallback(EventCallback cb) {
    _onEvent = cb;
}

void chrDisp::setBlinkCallback(BlinkCallback cb) {
    _onBlink = cb;
}

void chrDisp::setTurnOff(uint32_t turnOffMs) {
    if (turnOffMs > 0 && turnOffMs < MIN_TURN_OFF_MS) {
        turnOffMs = MIN_TURN_OFF_MS;
        _fireEvent(EVENT_NOTICE, "default turnoff min time used");
    }

    _turnOffMs = turnOffMs;
}

bool chrDisp::isOn() {
    return _isOn;
}

uint32_t chrDisp::getTurnOffMs() {
    return _turnOffMs;
}

void chrDisp::_fireEvent(int8_t code, const char* action) {
    if (!_onEvent) return;

    char msg[48];
    snprintf(msg, sizeof(msg), "disp [%u] %s", _id, action);
    
    _onEvent(code, msg);
}

void chrDisp::_drawFrameHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert, uint8_t frame) {
    if (_gfx == nullptr) return;
    if (frame == 0) return;

    uint16_t color = invert ? _bgColor : _fgColor;
    uint16_t background = invert ? _fgColor : _bgColor;

    int16_t frameX = x - frame;
    int16_t frameY = y - frame;
    int16_t frameWidth = width + 2 * frame;
    int16_t frameHeight = height + 2 * frame;

    _gfx->fillRoundRect(frameX, frameY, frameWidth, frameHeight, ROUNDED_RECT_RADIUS, background);
    if (color == _bgColor) {
        // Draw a foreground colored frame around if the color is the background color
        _gfx->drawRoundRect(frameX, frameY, frameWidth, frameHeight, ROUNDED_RECT_RADIUS, color);
    }
}

void chrDisp::_drawVFilledRectHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert, uint8_t percent, int16_t outlineRadius, bool batteryTip) {
    if (_gfx == nullptr) return;

    if (percent > 100) percent = 100;
    uint16_t color = invert ? _bgColor : _fgColor;

    // Battery tip size
    uint16_t tipHeight = 0;
    
    if (batteryTip) {
        uint16_t tipWidth = width / 2;
        tipHeight = height / 10;
        if (tipHeight < 1) tipHeight = 1;
        
        _gfx->fillRect(x + (width / 2) - (tipWidth / 2), y, tipWidth, tipHeight, color);
    }

    // Body size
    uint16_t bodyHeight = height - tipHeight - 1;
    uint16_t bodyWidth = width;
        
    // Body outline
    if (outlineRadius > 0) {
        _gfx->drawRoundRect(x, y + tipHeight, bodyWidth, bodyHeight, outlineRadius, color);
    } else {
        _gfx->drawRect(x, y + tipHeight, bodyWidth, bodyHeight, color);
    }
    
    
    // Fill
    if (bodyHeight < 5) return; // Safety check
    if (bodyWidth < 5) return;  // Safety check
    
    uint16_t fillHeight = map(constrain(percent, 0, 100), 0, 100, 0, bodyHeight - 4);
    
    _gfx->fillRect(
      x + 2, 
      y + tipHeight + 2 + (bodyHeight - 4 - fillHeight), 
      bodyWidth - 4, 
      fillHeight, 
      color
    );
}

void chrDisp::_drawHFilledRectHelper(int16_t x, int16_t y, uint16_t width, uint16_t height, bool invert, uint8_t percent, int16_t outlineRadius) {
    if (_gfx == nullptr) return;

    if (percent > 100) percent = 100;
    uint16_t color = invert ? _bgColor : _fgColor;

    // Frame
    if (outlineRadius > 0) {
        _gfx->drawRoundRect(x, y, width, height, outlineRadius, color);
    } else {
        _gfx->drawRect(x, y, width, height, color);
    }

    // Fill
    if (width < 5) return;   // Safety check
    if (height < 5) return;  // Safety check

    uint16_t fillWidth = map(constrain(percent, 0, 100), 0, 100, 0, width - 4);

    _gfx->fillRect(
        x + 2,
        y + 2,
        fillWidth,
        height - 4,
        color
    );
}

void chrDisp::drawVBar(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t percent) {
    if (_gfx == nullptr) return;
    _needRedraw = true;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // Do not draw during this phase of blinking

    _drawVFilledRectHelper(x, y, width, height, invert, percent, ROUNDED_RECT_RADIUS);
}

void chrDisp::drawHBar(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t percent) {
    if (_gfx == nullptr) return;
    _needRedraw = true;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // Do not draw during this phase of blinking

    _drawHFilledRectHelper(x, y, width, height, invert, percent, ROUNDED_RECT_RADIUS);
}

void chrDisp::drawBattery(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t percent) {
    if (_gfx == nullptr) return;
    _needRedraw = true;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // Do not draw during this phase of blinking

    _drawVFilledRectHelper(x, y, width, height, invert, percent, ROUNDED_RECT_RADIUS, true);
}

void chrDisp::drawCharge(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t notUsed) {
    (void)notUsed;
    if (_gfx == nullptr) return;
    _needRedraw = true;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // Do not draw during this phase of blinking

    uint16_t color = invert ? _bgColor : _fgColor;

    // 1. Determine the midpoints (using integer division to handle parity)
    // If width = 9 (odd), both will be 4 -> overlap
    // If width = 8 (even), upperEndX = 3, lowerStartX = 4 -> no overlap
    uint16_t upperEndX = x + (width - 1) / 2;
    uint16_t lowerStartX = x + width / 2;
    
    // Same for the height
    uint16_t upperEndY = y + (height - 1) / 2;
    uint16_t lowerStartY = y + height / 2;
    
    // Transform right-angled triangles into a lightning bolt shape
    // - move the upper left bottom corner down
    // - move the lower right top corner up
    uint16_t flashOffset = height / 6;
    
    // 2. Upper triangle (right angle on the right and the apex)
    // Apex: top center
    // Right angle: center, on the horizontal dividing line
    // Left corner: left edge of the area, on the dividing line
    _gfx->fillTriangle(
        upperEndX, y,              // Upper apex
        x, upperEndY + flashOffset,// Bottom left corner
        upperEndX, upperEndY,      // Bottom right (right angle)
        color
    );

    // 3. Lower triangle (right angle on the left and the apex)
    // Right angle: center, on the dividing line
    // Right corner: right edge of the area, on the dividing line
    // Apex: bottom center
    _gfx->fillTriangle(
        lowerStartX, lowerStartY,  // Top left (right angle)
        x + width - 1, lowerStartY - flashOffset, // Top right corner
        lowerStartX, y + height - 1, // Bottom apex
        color
    );
}

void chrDisp::drawCrosshair(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t notUsed) {
    (void)notUsed;
    if (_gfx == nullptr) return;
    _needRedraw = true;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // Do not draw during this phase of blinking

    uint16_t color = invert ? _bgColor : _fgColor;
    uint16_t centerX = x + (width / 2);
    uint16_t centerY = y + (height / 2);
    
    // Horizontal line
    _gfx->drawLine(x, centerY, x + (width - 1), centerY, color);
    
    // Vertical line
    _gfx->drawLine(centerX, y, centerX, y + (height - 1), color);
    
    // Filled circle in the middle
    uint16_t radius = min(width, height) / 4;
    if (radius < 1) radius = 1; // Min sugár
    _gfx->fillCircle(centerX, centerY, radius, color);
}

void chrDisp::drawDisk(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t percent) {
    if (_gfx == nullptr) return;
    _needRedraw = true;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // Do not draw during this phase of blinking

    // Color constant
    uint16_t color = invert ? _bgColor : _fgColor;

    // Scaling the top-right notch
    uint16_t notchSize = min(width, height) / 4;
    if (notchSize < 2) notchSize = 2;
    // Frequently used points
    uint16_t rightX = x + (width - 1);
    uint16_t notchLeftX = x + (width - 1) - notchSize;
    uint16_t notchRightY = y + notchSize;
    uint16_t bottomY = y + (height - 1);
  
    // Sides from the top-left corner clockwise
    _gfx->drawLine(x, y, notchLeftX, y, color);
    _gfx->drawLine(notchLeftX, y, rightX, notchRightY, color);
    _gfx->drawLine(rightX, notchRightY, rightX, bottomY, color);
    _gfx->drawLine(rightX, bottomY, x, bottomY, color);
    _gfx->drawLine(x, bottomY, x, y, color);

    // Circle in the center
    uint16_t centerX = x + (width / 2);
    uint16_t centerY = y + (height / 2) + 1; // Looks better if shifted one pixel down
    uint16_t radius = notchSize / 2;
    _gfx->drawCircle(centerX, centerY, radius, color);
    _gfx->fillCircle(centerX, centerY, radius - 1, color);

    // Precise scaling for process or saturation indicator
    uint16_t barTopY = centerY + radius + 1;
    
    // Safety check to ensure there is actually space to draw (on smaller displays)
    if (bottomY > barTopY + 2) {
        uint16_t barHeight = bottomY - barTopY - 1;
        drawVBar(x + 2, barTopY, width - 4, barHeight, blink, invert, 0, percent);
    }
}

void chrDisp::drawWiFi(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t percent) {
    if (_gfx == nullptr) return;
    _needRedraw = true;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // Do not draw during this phase of blinking

    uint16_t color = invert ? _bgColor : _fgColor;
    uint16_t bottomY = y + height - 1;
    
    if (percent > 100) percent = 100; // Safety limit
    uint8_t level = percent == 100 ? 3 : percent / 25;
    
    // Width division: 3 pillars and 2 gaps (approximately equal ratio)
    uint16_t pillarW = max((int)1, width / 5); 
    
    // The gap is the remaining space divided by two, ensuring it does not protrude from the box
    uint16_t gap = max((int)1, (width - (3 * pillarW)) / 2); 
    
    // The rounding radius is half the pillar width (perfect semicircle at the top and bottom)
    uint16_t radius = pillarW / 2;

    for (uint8_t i = 0; i < 3; i++) {
        // Height: 1/3, 2/3, and 3/3 (full height)
        uint16_t pillarH = max((int)3, ((i + 1) * height) / 3); 
        
        // X position: Offset by the width of the previous pillars and gaps
        uint16_t px = x + (i * (pillarW + gap)); 
        
        // Y position: The top of the pillar (aligned to the bottom)
        uint16_t py = bottomY - pillarH + 1;

        // If the pillar index is less than the signal strength, it is filled, otherwise empty
        if (i < level) {
            _gfx->fillRoundRect(px, py, pillarW, pillarH, radius, color);
        } else {
            _gfx->drawRoundRect(px, py, pillarW, pillarH, radius, color);
        }
        
        // If this is the first pillar and the signal strength is 0, draw an "X" above it
        if (i == 0 && percent == 0) {
            // Calculate the free space above the first pillar
            uint16_t spaceAbove = py - y; 
            
            // The size of the X should be proportional to the available space or the pillar
            uint16_t xSize = max((int)1, min((int)pillarW, (int)spaceAbove) / 2);
            
            // The center of the X is above the first pillar, in the middle of the free space
            uint16_t cx = px + (pillarW / 2);
            uint16_t cy = y + (spaceAbove / 2);

            _gfx->drawLine(cx - xSize, cy - xSize, cx + xSize, cy + xSize, color);
            _gfx->drawLine(cx - xSize, cy + xSize, cx + xSize, cy - xSize, color);
        }
    }
}

void chrDisp::drawAP(int16_t x, int16_t y, uint16_t width, uint16_t height, uint8_t blink, bool invert, uint8_t frame, uint8_t notUsed) {
    (void)notUsed;
    if (_gfx == nullptr) return;
    _needRedraw = true;
    
    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (if the entire display is not being refreshed)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return;  // No need to draw during this phase of blinking

    uint16_t color = invert ? _bgColor : _fgColor;
    uint16_t bottomY = y + height - 1;
    
    uint16_t centerX = x + (width / 2);
    uint16_t centerY = y + (height / 2);
    uint16_t maxRadius = min(width, height) / 2;
    uint16_t apDotRadius = max((int)1, maxRadius / 4);

    _gfx->drawLine(centerX, centerY, centerX - maxRadius / 2, bottomY, color);
    _gfx->drawLine(centerX, centerY, centerX + maxRadius / 2, bottomY, color);
    _gfx->fillCircle(centerX, centerY, apDotRadius, color);
    _gfx->drawCircleHelper(centerX, centerY, apDotRadius + (maxRadius - apDotRadius) / 2, 1 | 2, color);
    _gfx->drawCircleHelper(centerX, centerY, maxRadius, 1 | 2, color);
}

void chrDisp::drawPotmeter(int16_t x, int16_t y, uint16_t width, uint16_t zeroPointDeg, uint8_t blink, bool invert, uint8_t frame, uint8_t level) {
    if (_gfx == nullptr) return;
    _needRedraw = true;

    // Since the base is circular, the height of the area is the same as the width
    uint16_t height = width;

    if (frame != 0) {
        _drawFrameHelper(x, y, width, height, invert, frame);
    } else {
        // Clear the icon area (square area)
        _gfx->fillRect(x, y, width, height, invert ? _fgColor : _bgColor);
    }

    if (!checkBlinkBit(blink, _blinkQuarter)) return; // No need to draw during this phase of blinking

    uint16_t color = invert ? _bgColor : _fgColor;

    // Center of the circle
    int16_t centerX = x + (width / 2);
    int16_t centerY = y + (height / 2);
    
    // Determine the radius (so that it fits within the given width)
    int16_t radius = (width / 2) - 1;
    if (radius < 2) radius = 2; // Safety minimum size

    // Draw the outer circle
    _gfx->drawCircle(centerX, centerY, radius, color);

    // Calculate the angle based on your logic
    float angleDeg;
    if (zeroPointDeg < 360) {
        // 0-359: level will be added to zeroPointDeg
        angleDeg = zeroPointDeg + ((float)level * 1.2f);   // level * 1.25, to fill better the circle (306 degrees instead of 256)
    } else {
        // 360 vagy nagyobb érték esetén: arányos elosztás a teljes köríven
        float startAngle = zeroPointDeg % 360;
        angleDeg = startAngle + ((float)level * 360.0f / 256.0f);
    }

    // Convert to radians
    // (0 degrees is at the top, and it increases clockwise in our coordinate system)
    float angleRad = angleDeg * (M_PI / 180.0f);

    // Calculate the end point of the pointer on the circular arc (radius - 1 to stay within the circle outline)
    int16_t endX = centerX + round((radius - 1) * sin(angleRad));
    int16_t endY = centerY - round((radius - 1) * cos(angleRad));

    // Draw the pointer line from the center to the circular arc
    _gfx->drawLine(centerX, centerY, endX, endY, color);
}

void chrDisp::_drawTextHelper(int16_t x, int16_t y, uint8_t size, const char* text, uint8_t width, uint8_t blink, int16_t blinkChar, int16_t invert, const char* skipChars, uint8_t frame) {
    if (text == nullptr) return;    // Do not draw if the text is null

    if (_gfx == nullptr) return;
    _needRedraw = true;

    int16_t textX, textY;
    uint16_t textWidth, textHeight;
    _gfx->setTextSize(size);
    _gfx->getTextBounds(text, x, y, &textX, &textY, &textWidth, &textHeight);

    if (width > 0 && textWidth < width) {
        // Center the text within the given width if it's smaller than the width
        x += (width - textWidth) / 2;
        textWidth = width;  // Increase the frame or fillRect width to match the specified width
    }

    if (frame != 0) {
        _drawFrameHelper(textX, textY, textWidth, textHeight, invert, frame);
    } else {
        // Clear the icon area (square area)
        _gfx->fillRect(textX, textY, textWidth, textHeight, invert ? _fgColor : _bgColor);
    }

    bool fullBlink = blinkChar == 0 || blinkChar == -1;
    bool noLit = !checkBlinkBit(blink, _blinkQuarter);
    if (fullBlink && noLit) {
        // No need to draw during this phase of blinking
        return;
    }

    bool fullInvert = invert == -1;
    uint16_t color = fullInvert ? _bgColor : _fgColor;
    uint16_t background = fullInvert ? _fgColor : _bgColor;
    
    _gfx->setCursor(x, y);
    _gfx->setTextWrap(false);
    
    uint8_t charCounter = 0;    // For invert and blink count chars which are not skipped
    for (uint16_t i = 0; text[i] != '\0'; ++i) {
        if (skipChars == nullptr || strchr(skipChars, text[i]) == nullptr) ++charCounter;

        // Determine if this character should be lit or not
        if (noLit
            && (
                ( blinkChar > 0 && (blinkChar == charCounter) )               // Blink the specified char only
                || ( blinkChar < -1 && (charCounter % (-blinkChar) == 0) )  // Blink every (-blinkChar)th character
            )
        ) {
            _gfx->print(' ');  // Print a space instead of the character to simulate blinking off
            continue;          // Skip the rest of the loop for this character
        }

        // Determine if this character should be inverted or not
        bool invertChar = fullInvert;   // Non or every character inversion based on the value of 'invert'
        if (invert > 0) {
            // Change inversion only for the specified char
            invertChar = invert == charCounter;
        } else if (invert < -1) {
            // Change inversion only for every (-invert)th character
            invertChar = charCounter % (-invert) == 0;
        }

        _gfx->setTextColor(invertChar ? background : color, invertChar ? color : background);
        _gfx->print(text[i]);
    }
}

bool chrDisp::loop(bool turnOnResetSleep, bool allowStoreForAutoBlink, const DisplayItem* items, uint8_t itemCount, bool invertDisplay, bool clearDisplay) {
    if (_gfx == nullptr) {
        if (_firstLoop) {
            _fireEvent(chrDisp::EVENT_ERR, "GFX obj not initialized");
        }
        _firstLoop = false;

        return false;
    }

    if (turnOnResetSleep) on();           // Turn on or just refresh the last turn-on time
    
    uint32_t now = millis();
    if (_isOn && (_turnOffMs > 0) && (now - _lastOnMillis >= _turnOffMs) ) {
        // The valid _turnOffMs has expired, the display can be turned off
        off();
    }

    if (allowStoreForAutoBlink == false && (_lastAutoBlinkItems != nullptr || _lastAutoBlinkItemsCount > 0)) {
        // Tárolás törlése, ha nem engedélyezett
        _lastAutoBlinkItems = nullptr;
        _lastAutoBlinkItemsCount = 0;
    }

    if (!_isOn) return false;   // Ha ezek után nincs bekapcsolva a kijelző, akkor nincs további teendő

    if (_isOn && (_turnOffMs > DIM_TIME_MS) && (now - _lastOnMillis >= _turnOffMs - DIM_TIME_MS) ) {
        // Only DIM_TIME_MS remains from the valid _turnOffMs, the display can be dimmed
        dim(true);
    } else {
        dim(false);
    }

    // Blinking timing (calculate only if BLINK_INTERVAL_MS passed)
    uint8_t oldBlinkQuarter = _blinkQuarter;
    uint32_t elapsedSincePrevQuarter = now - _blinkQuarterChangedMillis;
    if (elapsedSincePrevQuarter > BLINK_INTERVAL_MS) {
        uint32_t elapsedQuarters = elapsedSincePrevQuarter / BLINK_INTERVAL_MS;
        _blinkQuarter = (_blinkQuarter + elapsedQuarters) % 8;
        _blinkQuarterChangedMillis += elapsedQuarters * BLINK_INTERVAL_MS;

        // Fire the blink callback if it is set
        if (_onBlink) _onBlink(_id, _blinkQuarter);
    }
    
    if (items != nullptr && itemCount > 0) {
      // The screen needs to be redrawn because the user sent the items
      _needRedraw = true;
      if (clearDisplay) clear();

      if (allowStoreForAutoBlink) {
        // Store the data for possible blinking
        _lastAutoBlinkItems = items;
        _lastAutoBlinkItemsCount = itemCount;
      }
    } else {
      // It would not be necessary to redraw the screen, but should check if blinking items need to change state

      if (oldBlinkQuarter != _blinkQuarter && _lastAutoBlinkItems != nullptr && _lastAutoBlinkItemsCount > 0) {
        // There is a previously saved item, the question is whether something needs to blink

        for (uint8_t i = 0; i < _lastAutoBlinkItemsCount; i++) {
          if (_lastAutoBlinkItems[i].blink != 0b11111111) {
            // There is an item waiting to blink

            bool currentOn = checkBlinkBit(_lastAutoBlinkItems[i].blink, oldBlinkQuarter);
            bool nextOn = checkBlinkBit(_lastAutoBlinkItems[i].blink, _blinkQuarter);

            if (currentOn != nextOn) {
              // The item waiting to blink will now be in a different state than it was, so a redraw is needed
              _needRedraw = true;
              if (clearDisplay) clear();

              items = _lastAutoBlinkItems;
              itemCount = _lastAutoBlinkItemsCount;

              break;
            }
          }
        }
      }
    }
    
    bool refreshed = false;
    if (_needRedraw) {
      // Refresh content

        if (items != nullptr) {
          for (uint8_t i = 0; i < itemCount; i++) {
            if (items[i].type == ITEM_TEXT) {
                _drawTextHelper(items[i].x, items[i].y, items[i].size, items[i].text, items[i].width, items[i].blink, items[i].blinkChar, items[i].invert, items[i].skipChars, items[i].frame);
            } else {
                drawItem(items[i].type, items[i].x, items[i].y, items[i].width, items[i].size, items[i].blink, items[i].invert != 0, items[i].frame, items[i].data);
            }
          }
      }

      if (_hwUpdate) { _hwUpdate(); }
      refreshed = true;
    }
  
    if (_needRedraw || (now - _lastRedrawMillis > BLINK_INTERVAL_MS)) {
      // Inversion is only performed at BLINK_INTERVAL_MS intervals; if the screen did not need to be refreshed, this ensures that calling the loop with true once and then immediately with false will invert the screen for at least BLINK_INTERVAL_MS
      if (invert(invertDisplay)) {
          refreshed = true;
      }
    }

    if (_needRedraw) {
      _needRedraw = false;
    }
    if (refreshed) {
        _lastRedrawMillis = now;
        _fireEvent(EVENT_REFRESHED, "refreshed");
    }
    return refreshed;
}

bool chrDisp::loop(bool turnOnResetSleep, const char* text, uint8_t size, bool invertDisplay, bool clearDisplay) {
    DisplayItem items[1] = { text, size };
    return loop(turnOnResetSleep, false, items, 1, invertDisplay, clearDisplay);
}

bool chrDisp::loop(bool turnOnResetSleep) {
    return loop(turnOnResetSleep, _lastAutoBlinkItems != nullptr && _lastAutoBlinkItemsCount > 0, (const DisplayItem*)nullptr, 0);   // Ha van elmentett AutoBlinkItems, megtartja
}
