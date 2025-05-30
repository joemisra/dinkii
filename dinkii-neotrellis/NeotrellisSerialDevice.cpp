#include "NeotrellisSerialDevice.h"
#include "debug.h"

NeotrellisSerialDevice::NeotrellisSerialDevice() : MonomeSerialDevice()
{
    animationActive = false;
    animationStartTime = 0;
    animationDuration = 0;
    animationStep = 0;
}

void NeotrellisSerialDevice::initialize(Adafruit_MultiTrellis *trellis, uint8_t rows, uint8_t columns)
{
    // Initialize the base class
    MonomeSerialDevice::initialize();
    _trellis = trellis;

    // Set up the monome device as a grid
    setupAsGrid(rows, columns);

    // Initialize all LEDs to off and default color to white
    for (int i = 0; i < MAXLEDCOUNT; i++)
    {
        ledColors[i].r = 255;
        ledColors[i].g = 255;
        ledColors[i].b = 255;
        brightness[i] = 0;
    }
}

uint16_t NeotrellisSerialDevice::getLedIndex(uint8_t x, uint8_t y)
{
    if (x < columns && y < rows)
    {
        return y * columns + x;
    }
    return 0; // Default to first LED if out of bounds
}

uint8_t NeotrellisSerialDevice::scaleMonoBrightness(uint8_t monoLevel)
{
    // Scale 0-15 to 0-255
    return map(monoLevel, 0, 15, 0, 255);
}

void NeotrellisSerialDevice::setRGBLed(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t index = getLedIndex(x, y);
    if (index < MAXLEDCOUNT)
    {
        ledColors[index].r = r;
        ledColors[index].g = g;
        ledColors[index].b = b;
        // Keep current brightness
    }
}

void NeotrellisSerialDevice::setRGBLedBrightness(uint8_t x, uint8_t y, uint8_t brightness_val)
{
    uint16_t index = getLedIndex(x, y);
    if (index < MAXLEDCOUNT)
    {
        brightness[index] = brightness_val;
    }
}

// Override from MonomeSerialDevice
void NeotrellisSerialDevice::setGridLed(uint8_t x, uint8_t y, uint8_t level)
{
    // Call the parent method to maintain compatibility
    MonomeSerialDevice::setGridLed(x, y, level);

    // Update our brightness array with the expanded range
    uint16_t index = getLedIndex(x, y);
    if (index < MAXLEDCOUNT)
    {
        brightness[index] = scaleMonoBrightness(level);
    }
}

void NeotrellisSerialDevice::clearGridLed(uint8_t x, uint8_t y)
{
    setGridLed(x, y, 0);
}

void NeotrellisSerialDevice::setAllLEDs(int value)
{
    // Call the parent method to maintain compatibility
    MonomeSerialDevice::setAllLEDs(value);

    // Update our brightness array with the expanded range
    uint8_t scaledValue = scaleMonoBrightness(value);
    for (int i = 0; i < MAXLEDCOUNT; i++)
    {
        brightness[i] = scaledValue;
    }
}

void NeotrellisSerialDevice::clearAllLeds()
{
    setAllLEDs(0);
}

void NeotrellisSerialDevice::updateTrellis()
{
    if (!_trellis)
        return;

    bool updated = false;

    // Update all LEDs
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < columns; x++)
        {
            uint16_t index = getLedIndex(x, y);
            uint16_t trellis_index = x + y * columns;

            // Calculate the color based on brightness level
            uint8_t r = map(brightness[index], 0, 255, 0, ledColors[index].r);
            uint8_t g = map(brightness[index], 0, 255, 0, ledColors[index].g);
            uint8_t b = map(brightness[index], 0, 255, 0, ledColors[index].b);

            // Set the color on the trellis
            uint32_t color = (uint32_t)r << 16 | (uint32_t)g << 8 | b;
            _trellis->setPixelColor(trellis_index, color);
            updated = true;
        }
    }

    // Only call show() if we've updated something
    if (updated)
    {
        _trellis->show();
    }
}

void NeotrellisSerialDevice::startPulseAnimation(uint16_t duration)
{
    animationActive = true;
    animationStartTime = millis();
    animationDuration = duration;
    animationStep = 0;
}

void NeotrellisSerialDevice::updateAnimations()
{
    if (!animationActive)
        return;

    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - animationStartTime;

    if (elapsed >= animationDuration)
    {
        // Reset animation or make it loop
        animationStartTime = currentTime;
        elapsed = 0;
    }

    // Calculate the brightness level (0-255) based on sine wave
    // This creates a smooth pulse effect
    float progress = (float)elapsed / animationDuration;
    float angle = progress * 2 * PI;
    uint8_t value = (sin(angle) + 1) * 127; // Map -1.0,1.0 to 0,255

    // For a rainbow color cycle
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < columns; x++)
        {
            uint16_t index = getLedIndex(x, y);

            // Create a rainbow pattern that moves with time
            float hue = (float)(x + y) / (columns + rows) + progress;
            if (hue > 1.0f)
                hue -= 1.0f;

            // Simple HSV to RGB conversion
            if (hue < 1.0f / 6.0f)
            {
                ledColors[index].r = 255;
                ledColors[index].g = 255 * 6 * hue;
                ledColors[index].b = 0;
            }
            else if (hue < 2.0f / 6.0f)
            {
                ledColors[index].r = 255 * (2.0f - 6 * hue);
                ledColors[index].g = 255;
                ledColors[index].b = 0;
            }
            else if (hue < 3.0f / 6.0f)
            {
                ledColors[index].r = 0;
                ledColors[index].g = 255;
                ledColors[index].b = 255 * (6 * hue - 2.0f);
            }
            else if (hue < 4.0f / 6.0f)
            {
                ledColors[index].r = 0;
                ledColors[index].g = 255 * (4.0f - 6 * hue);
                ledColors[index].b = 255;
            }
            else if (hue < 5.0f / 6.0f)
            {
                ledColors[index].r = 255 * (6 * hue - 4.0f);
                ledColors[index].g = 0;
                ledColors[index].b = 255;
            }
            else
            {
                ledColors[index].r = 255;
                ledColors[index].g = 0;
                ledColors[index].b = 255 * (6.0f - 6 * hue);
            }

            // Set brightness using the pulsing value
            brightness[index] = value;
        }
    }

    // Update the trellis with our new values
    updateTrellis();
}