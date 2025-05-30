#ifndef NEOTRELLIS_SERIAL_DEVICE_H
#define NEOTRELLIS_SERIAL_DEVICE_H

#include "MonomeSerialDevice.h"
#include <Adafruit_NeoTrellis.h>

struct RGBColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class NeotrellisSerialDevice : public MonomeSerialDevice
{
public:
    NeotrellisSerialDevice();
    void initialize(Adafruit_MultiTrellis *trellis, uint8_t rows, uint8_t columns);

    // RGB LED control methods
    void setRGBLed(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
    void setRGBLedBrightness(uint8_t x, uint8_t y, uint8_t brightness);

    // Update LEDs to trellis
    void updateTrellis();

    // Animation methods
    void startPulseAnimation(uint16_t duration = 2000);
    void updateAnimations();

    // Override methods from MonomeSerialDevice
    void setGridLed(uint8_t x, uint8_t y, uint8_t level);
    void clearGridLed(uint8_t x, uint8_t y);
    void setAllLEDs(int value);
    void clearAllLeds();

private:
    Adafruit_MultiTrellis *_trellis;
    RGBColor ledColors[MAXLEDCOUNT];
    uint8_t brightness[MAXLEDCOUNT];

    // Animation state
    bool animationActive;
    uint32_t animationStartTime;
    uint16_t animationDuration;
    uint8_t animationStep;

    // Convert monome brightness level (0-15) to full range (0-255)
    uint8_t scaleMonoBrightness(uint8_t monoLevel);

    // Get LED index from x,y coordinates
    uint16_t getLedIndex(uint8_t x, uint8_t y);
};

#endif // NEOTRELLIS_SERIAL_DEVICE_H