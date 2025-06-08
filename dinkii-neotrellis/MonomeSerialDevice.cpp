#include "MonomeSerialDevice.h"
#include "debug.h"

MonomeSerialDevice::MonomeSerialDevice() {}

void MonomeSerialDevice::initialize()
{
  active = false;
  isMonome = false;
  isGrid = true;
  rows = 0;
  columns = 0;
  encoders = 0;
  // clearQueue();
  clearAllLeds();
  arcDirty = false;
  gridDirty = false;
}

void MonomeSerialDevice::setupAsGrid(uint8_t _rows, uint8_t _columns)
{
  initialize();
  active = true;
  isMonome = true;
  isGrid = true;
  rows = _rows;
  columns = _columns;
  gridDirty = true;
  debugfln(INFO, "GRID rows: %d columns %d", rows, columns);
}

void MonomeSerialDevice::setupAsArc(uint8_t _encoders)
{
  initialize();
  active = true;
  isMonome = true;
  isGrid = false;
  encoders = _encoders;
  arcDirty = true;
  debugfln(INFO, "ARC encoders: %d", encoders);
}

void MonomeSerialDevice::getDeviceInfo()
{
  // debugln(INFO, "MonomeSerialDevice::getDeviceInfo");
  Serial.write(uint8_t(0));
}

void MonomeSerialDevice::poll()
{
  // while (isMonome && Serial.available()) { processSerial(); };
  if (Serial.available())
  {
    processSerial();
  }
  // Serial.println("processSerial");
}

void MonomeSerialDevice::setAllLEDs(int value)
{
  for (int i = 0; i < MAXLEDCOUNT; i++)
    leds[i] = value;
}

void MonomeSerialDevice::setGridLed(uint8_t x, uint8_t y, uint8_t level)
{
  //    int index = x + (y * columns);
  //    if (index < MAXLEDCOUNT) leds[index] = level;

  if (x < columns && y < rows)
  {
    uint32_t index = y * columns + x;
    leds[index] = level;
  }
  // debugfln(INFO, "LED index: %d x %d y %d", index, x, y);
}

void MonomeSerialDevice::clearGridLed(uint8_t x, uint8_t y)
{
  setGridLed(x, y, 0);
  // Serial.println("clearGridLed");
}

void MonomeSerialDevice::setArcLed(uint8_t enc, uint8_t led, uint8_t level)
{
  int index = led + (enc << 6);
  if (index < MAXLEDCOUNT)
    leds[index] = level;
  // Serial.println("setArcLed");
}

void MonomeSerialDevice::clearArcLed(uint8_t enc, uint8_t led)
{
  setArcLed(enc, led, 0);
  // Serial.println("clearArcLed");
}

void MonomeSerialDevice::clearAllLeds()
{
  for (int i = 0; i < MAXLEDCOUNT; i++)
    leds[i] = 0;
  // Serial.println("clearAllLeds");
}

void MonomeSerialDevice::clearArcRing(uint8_t ring)
{
  for (int i = ring << 6, upper = i + 64; i < upper; i++)
    leds[i] = 0;
  // Serial.println("clearArcRing");
}

void MonomeSerialDevice::refreshGrid()
{
  gridDirty = true;
  // Serial.println("refreshGrid");
}

void MonomeSerialDevice::refreshArc()
{
  arcDirty = true;
  // Serial.println("refreshArc");
}

void MonomeSerialDevice::refresh()
{
  /*
      uint8_t buf[35];
      int ind, led;

      if (gridDirty) {
          //Serial.println("gridDirty");
          buf[0] = 0x1A;
          buf[1] = 0;
          buf[2] = 0;

          ind = 3;
          for (int y = 0; y < 8; y++)
              for (int x = 0; x < 8; x += 2) {
                  led = (y << 4) + x;
                  buf[ind++] = (leds[led] << 4) | leds[led + 1];
              }
          Serial.write(buf, 35);

          ind = 3;
          buf[1] = 8;
          for (int y = 0; y < 8; y++)
              for (int x = 8; x < 16; x += 2) {
                  led = (y << 4) + x;
                  buf[ind++] = (leds[led] << 4) | leds[led + 1];
              }
          Serial.write(buf, 35);

          ind = 3;
          buf[1] = 0;
          buf[2] = 8;
          for (int y = 8; y < 16; y++)
              for (int x = 0; x < 8; x += 2) {
                  led = (y << 4) + x;
                  buf[ind++] = (leds[led] << 4) | leds[led + 1];
              }
          Serial.write(buf, 35);

          ind = 3;
          buf[1] = 8;
          for (int y = 8; y < 16; y++)
              for (int x = 8; x < 16; x += 2) {
                  led = (y << 4) + x;
                  buf[ind++] = (leds[led] << 4) | leds[led + 1];
              }
          Serial.write(buf, 35);

          gridDirty = false;
      }

      if (arcDirty) {
          //Serial.print("arcDirty");
          buf[0] = 0x92;

          buf[1] = 0;
          ind = 2;
          for (led = 0; led < 64; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          buf[1] = 1;
          ind = 2;
          for (led = 64; led < 128; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          buf[1] = 2;
          ind = 2;
          for (led = 128; led < 192; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          buf[1] = 3;
          ind = 2;
          for (led = 192; led < 256; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          buf[1] = 4;
          ind = 2;
          for (led = 256; led < 320; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          buf[1] = 5;
          ind = 2;
          for (led = 320; led < 384; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          buf[1] = 6;
          ind = 2;
          for (led = 384; led < 448; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          buf[1] = 7;
          ind = 2;
          for (led = 448; led < 512; led += 2)
              buf[ind++] = (leds[led] << 4) | leds[led + 1];
          Serial.write(buf, 34);

          arcDirty = 0;
      }
  */
}

void MonomeSerialDevice::processSerial()
{
  // jherer
  if (Serial.available() < 1)
    return;

  uint8_t identifierSent = Serial.read(); // command byte from controller

  // System commands (0x00-0x0F)
  if (identifierSent <= 0x0F)
  {
    handleSystemCommand(identifierSent);
    return;
  }

  // LED grid commands (0x10-0x1F)
  if (identifierSent >= 0x10 && identifierSent <= 0x1F)
  {
    handleLedGridCommand(identifierSent);
    return;
  }

  // Key grid commands (0x20-0x21)
  if (identifierSent >= 0x20 && identifierSent <= 0x21)
  {
    handleKeyGridCommand(identifierSent);
    return;
  }

  // Encoder commands (0x50-0x52)
  if (identifierSent >= 0x50 && identifierSent <= 0x52)
  {
    handleEncoderCommand(identifierSent);
    return;
  }

  // Tilt commands (0x80-0x81)
  if (identifierSent >= 0x80 && identifierSent <= 0x81)
  {
    handleTiltCommand(identifierSent);
    return;
  }

  // LED ring commands (0x90-0x93)
  if (identifierSent >= 0x90 && identifierSent <= 0x93)
  {
    handleLedRingCommand(identifierSent);
    return;
  }

  // Custom RGB commands (0xC0-0xC9)
  if (identifierSent >= 0xC0 && identifierSent <= 0xC9)
  {
    handleRgbCommand(identifierSent);
    return;
  }
}

void MonomeSerialDevice::handleSystemCommand(uint8_t cmd)
{
  uint8_t i, readX, readY, dummy, gridNum, deviceAddress;
  uint8_t numQuads = columns / rows;

  switch (cmd)
  {
  case 0x00:                         // Device information query
    Serial.write((uint8_t)0x00);     // response code
    Serial.write((uint8_t)0x01);     // section id, 1 = led-grid
    Serial.write((uint8_t)numQuads); // number of quads (8x8 units)

    Serial.write((uint8_t)0x00);     // send again for key-grid
    Serial.write((uint8_t)0x02);     // section id, 2 = key-grid
    Serial.write((uint8_t)numQuads); // number of quads
    break;

  case 0x01:                     // System / ID
    Serial.write((uint8_t)0x01); // response code
    for (i = 0; i < 32; i++)
    { // send 32-byte device ID
      if (i < deviceID.length())
      {
        Serial.write(deviceID[i]);
      }
      else
      {
        Serial.write((uint8_t)0x00); // pad with zeros
      }
    }
    break;

  case 0x02: // System / write ID
    for (i = 0; i < 32; i++)
    {
      deviceID[i] = Serial.read();
    }
    break;

  case 0x03: // System / report grid offset
    Serial.write((uint8_t)0x02);
    Serial.write((uint8_t)0x01);
    Serial.write((uint8_t)0); // x offset
    Serial.write((uint8_t)0); // y offset
    break;

  case 0x04:                 // System / report ADDR
    gridNum = Serial.read(); // grid number
    readX = Serial.read();   // x offset
    readY = Serial.read();   // y offset
    break;

  case 0x05:                        // System / get grid size
    Serial.write((uint8_t)0x03);    // response code
    Serial.write((uint8_t)columns); // width
    Serial.write((uint8_t)rows);    // height
    break;

  case 0x06: // System / set grid size (ignored)
    readX = Serial.read();
    readY = Serial.read();
    break;

  case 0x07: // I2C get addr (ignored)
    break;

  case 0x08: // I2C set addr (ignored)
    deviceAddress = Serial.read();
    dummy = Serial.read();
    break;

  case 0x0F: // System / report firmware version
    for (i = 0; i < 8; i++)
    {
      Serial.read(); // Read but ignore firmware version
    }
    break;
  }
}

void MonomeSerialDevice::handleLedGridCommand(uint8_t cmd)
{
  uint8_t readX, readY, intensity;
  uint8_t x, y, z;

  switch (cmd)
  {
  case 0x10: // LED off: /prefix/led/set x y 0
    readX = Serial.read();
    readY = Serial.read();
    setGridLed(readX, readY, 0);
    break;

  case 0x11: // LED on: /prefix/led/set x y 1
    readX = Serial.read();
    readY = Serial.read();
    setGridLed(readX, readY, 15); // full brightness
    break;

  case 0x12: // All LEDs off: /prefix/led/all 0
    clearAllLeds();
    break;

  case 0x13:        // All LEDs on: /prefix/led/all 1
    setAllLEDs(15); // full brightness
    break;

  case 0x14: // LED map (8x8 frame): /prefix/led/map x y d[8]
    readX = Serial.read();
    while (readX > 16)
      readX += 16; // handle negative values
    readX &= 0xF8; // floor to 0 or 8

    readY = Serial.read();
    while (readY > 16)
      readY += 16; // handle negative values
    readY &= 0xF8; // floor to 0 or 8

    for (y = 0; y < 8; y++)
    {                            // each i will be a row
      intensity = Serial.read(); // read one byte of 8 bits on/off

      for (x = 0; x < 8; x++)
      { // for 8 LEDs on a row
        if ((intensity >> x) & 0x01)
        { // if intensity bit set, light led full brightness
          setGridLed(readX + x, readY + y, 15);
        }
        else
        {
          setGridLed(readX + x, readY + y, 0);
        }
      }
    }
    break;

  case 0x15:               //  /prefix/led/row x y d
    readX = Serial.read(); // led-grid / set row
    while (readX > 16)
      readX += 16; // handle negative values
    readX &= 0xF8; // floor to 0 or 8

    readY = Serial.read();     //
    intensity = Serial.read(); // read one byte of 8 bits on/off

    for (x = 0; x < 8; x++)
    { // for the next 8 lights in row
      if ((intensity >> x) & 0x01)
      { // if intensity bit set, light led full brightness
        setGridLed(readX + x, readY, 15);
      }
      else
      {
        setGridLed(readX + x, readY, 0);
      }
    }

    break;

  case 0x16:               //  /prefix/led/col x y d
    readX = Serial.read(); // led-grid / column set

    readY = Serial.read();
    while (readY > 16)
      readY += 16; // handle negative values
    readY &= 0xF8; // floor to 0 or 8

    intensity = Serial.read(); // read one byte of 8 bits on/off

    for (y = 0; y < 8; y++)
    { // for the next 8 lights in column
      if ((intensity >> y) & 0x01)
      { // if intensity bit set, light led full brightness
        setGridLed(readX, readY + y, 15);
      }
      else
      {
        setGridLed(readX, readY + y, 0);
      }
    }

    break;

  case 0x17:                   //  /prefix/led/intensity i
    intensity = Serial.read(); // set brightness for entire grid
    // this is probably not right
    setAllLEDs(intensity);

    break;

  case 0x18:                   //  /prefix/led/level/set x y i
    readX = Serial.read();     // led-grid / set LED intensity
    readY = Serial.read();     // read the x and y coordinates
    intensity = Serial.read(); // read the intensity
    setGridLed(readX, readY, intensity);
    break;

  case 0x19:                   //  /prefix/led/level/all s
    intensity = Serial.read(); // set all leds
    setAllLEDs(intensity);
    break;

  case 0x1A:               //   /prefix/led/level/map x y d[64]
                           // set 8x8 block
    readX = Serial.read(); // x offset
    while (readX > 16)
    {
      readX += 16;
    } // hacky shit to deal with negative numbers from rotation
    readX &= 0xF8;         // floor the offset to 0 or 8
    readY = Serial.read(); // y offset
    while (readY > 16)
    {
      readY += 16;
    } // hacky shit to deal with negative numbers from rotation
    readY &= 0xF8; // floor the offset to 0 or 8

    z = 0;
    for (y = 0; y < 8; y++)
    {
      for (x = 0; x < 8; x++)
      {
        if (z % 2 == 0)
        {
          intensity = Serial.read();
          if (((intensity >> 4) & 0x0F) > variMonoThresh)
          { // even bytes, use upper nybble
            setGridLed(readX + x, readY + y, (intensity >> 4) & 0x0F);
          }
          else
          {
            setGridLed(readX + x, readY + y, 0);
          }
        }
        else
        {
          if ((intensity & 0x0F) > variMonoThresh)
          { // odd bytes, use lower nybble
            setGridLed(readX + x, readY + y, intensity & 0x0F);
          }
          else
          {
            setGridLed(readX + x, readY + y, 0);
          }
        }
        z++;
      }
    }
    /*
     } else {
       for (int q = 0; q<32; q++){
         Serial.read();
       }
     }*/
    break;

  case 0x1B:               // /prefix/led/level/row x y d[8]
    readX = Serial.read(); // x offset
    while (readX > 16)
    {
      readX += 16;
    } // hacky shit to deal with negative numbers from rotation
    readX &= 0xF8;         // floor the offset to 0 or 8
    readY = Serial.read(); // y offset
    while (readY > 16)
    {
      readY += 16;
    } // hacky shit to deal with negative numbers from rotation
    readY &= 0xF8; // floor the offset to 0 or 8
    for (x = 0; x < 8; x++)
    {
      if (x % 2 == 0)
      {
        intensity = Serial.read();
        if ((intensity >> 4 & 0x0F) > variMonoThresh)
        { // even bytes, use upper nybble
          setGridLed(readX + x, readY, (intensity >> 4) & 0x0F);
        }
        else
        {
          setGridLed(readX + x, readY, 0);
        }
      }
      else
      {
        if ((intensity & 0x0F) > variMonoThresh)
        { // odd bytes, use lower nybble
          setGridLed(readX + x, readY, intensity & 0x0F);
        }
        else
        {
          setGridLed(readX + x, readY, 0);
        }
      }
    }
    break;

  case 0x1C:               // /prefix/led/level/col x y d[8]
    readX = Serial.read(); // x offset
    while (readX > 16)
    {
      readX += 16;
    } // hacky shit to deal with negative numbers from rotation
    readX &= 0xF8;         // floor the offset to 0 or 8
    readY = Serial.read(); // y offset
    while (readY > 16)
    {
      readY += 16;
    } // hacky shit to deal with negative numbers from rotation
    readY &= 0xF8; // floor the offset to 0 or 8
    for (y = 0; y < 8; y++)
    {
      if (y % 2 == 0)
      {
        intensity = Serial.read();
        if ((intensity >> 4 & 0x0F) > variMonoThresh)
        { // even bytes, use upper nybble
          setGridLed(readX, readY + y, (intensity >> 4) & 0x0F);
        }
        else
        {
          setGridLed(readX, readY + y, 0);
        }
      }
      else
      {
        if ((intensity & 0x0F) > variMonoThresh)
        { // odd bytes, use lower nybble
          setGridLed(readX, readY + y, intensity & 0x0F);
        }
        else
        {
          setGridLed(readX, readY + y, 0);
        }
      }
    }
    break;
  }
}

void MonomeSerialDevice::handleKeyGridCommand(uint8_t cmd)
{
  uint8_t gridKeyX, gridKeyY;

  switch (cmd)
  {
  case 0x20: // Key up: /prefix/grid/key x y 0
    gridKeyX = Serial.read();
    gridKeyY = Serial.read();
    // Instead of addGridEvent(gridKeyX, gridKeyY, 0);
    // Just directly implement what you need here
    // For example, if you need to send the event back:
    Serial.write((uint8_t)0x20);
    Serial.write(gridKeyX);
    Serial.write(gridKeyY);
    Serial.write((uint8_t)0);
    break;

  case 0x21: // Key down: /prefix/grid/key x y 1
    gridKeyX = Serial.read();
    gridKeyY = Serial.read();
    // Instead of addGridEvent(gridKeyX, gridKeyY, 1);
    Serial.write((uint8_t)0x21);
    Serial.write(gridKeyX);
    Serial.write(gridKeyY);
    Serial.write((uint8_t)1);
    break;
  }
}

void MonomeSerialDevice::handleEncoderCommand(uint8_t cmd)
{
  uint8_t index, n;
  int8_t delta;

  switch (cmd)
  {
  case 0x50: // Encoder delta: /prefix/enc/delta n d
    index = Serial.read();
    delta = Serial.read();
    // Instead of addArcEvent(index, delta);
    // Implement direct handling
    Serial.write((uint8_t)0x50);
    Serial.write(index);
    Serial.write(delta);
    break;

  case 0x51: // Encoder key up: /prefix/enc/key n 0
    n = Serial.read();
    // Handle encoder key up event if needed
    break;

  case 0x52: // Encoder key down: /prefix/enc/key n 1
    n = Serial.read();
    // Handle encoder key down event if needed
    break;
  }
}

void MonomeSerialDevice::handleTiltCommand(uint8_t cmd)
{
  // Not fully implemented in original code
  switch (cmd)
  {
  case 0x80: // Tilt active response
    break;

  case 0x81: // Tilt data
    break;
  }
}

void MonomeSerialDevice::handleLedRingCommand(uint8_t cmd)
{
  uint8_t readN, readX, readY, readA, intensity, x, y;

  switch (cmd)
  {
  case 0x90:               // Set LED: /prefix/ring/set n x a
    readN = Serial.read(); // ring number
    readX = Serial.read(); // LED number
    readA = Serial.read(); // value (0-15)
    setArcLed(readN, readX, readA);
    break;

  case 0x91:               // Set all LEDs: /prefix/ring/all n a
    readN = Serial.read(); // ring number
    readA = Serial.read(); // value (0-15)
    for (int q = 0; q < 64; q++)
    {
      setArcLed(readN, q, readA);
    }
    break;

  case 0x92:               // Map LEDs: /prefix/ring/map n d[32]
    readN = Serial.read(); // ring number
    for (y = 0; y < 64; y++)
    {
      if (y % 2 == 0)
      {
        intensity = Serial.read();
        // Even bytes, use upper nybble
        setArcLed(readN, y, (intensity >> 4 & 0x0F));
      }
      else
      {
        // Odd bytes, use lower nybble
        setArcLed(readN, y, (intensity & 0x0F));
      }
    }
    break;

  case 0x93:               // Set LED range: /prefix/ring/range n x1 x2 a
    readN = Serial.read(); // ring number
    readX = Serial.read(); // starting position (x1)
    readY = Serial.read(); // ending position (x2)
    readA = Serial.read(); // value (0-15)

    if (readX < readY)
    {
      // Simple range
      for (y = readX; y <= readY; y++)
      {
        setArcLed(readN, y, readA);
      }
    }
    else
    {
      // Wrapping range
      for (y = readX; y < 64; y++)
      {
        setArcLed(readN, y, readA);
      }
      for (y = 0; y <= readY; y++)
      {
        setArcLed(readN, y, readA);
      }
    }
    break;
  }
}

// Add these implementations:

void MonomeSerialDevice::setRgbColor(uint8_t r, uint8_t g, uint8_t b)
{
  ledR = r;
  ledG = g;
  ledB = b;
  // Flag as dirty to refresh display with new color
  gridDirty = true;
}

void MonomeSerialDevice::setBrightness(uint8_t brightness)
{
  globalBrightness = brightness;
  // Flag as dirty to refresh display with new brightness
  gridDirty = true;
}

// Add this to the processSerial method's switch cases:
// Add this new handler method:
void MonomeSerialDevice::handleRgbCommand(uint8_t cmd)
{
  uint8_t readX, readY, r, g, b, brightness;

  switch (cmd)
  {
  case 0xC0:           // Set global RGB color: /prefix/rgb/color r g b
    r = Serial.read(); // Red (0-255)
    g = Serial.read(); // Green (0-255)
    b = Serial.read(); // Blue (0-255)
    setRgbColor(r, g, b);
    break;

  case 0xC1:                    // Set global brightness: /prefix/rgb/brightness i
    brightness = Serial.read(); // Brightness (0-255)
    setBrightness(brightness);
    break;

  case 0xC2:               // Set per-key RGB: /prefix/rgb/key x y r g b
    readX = Serial.read(); // key x position
    readY = Serial.read(); // key y position
    r = Serial.read();     // Red (0-255)
    g = Serial.read();     // Green (0-255)
    b = Serial.read();     // Blue (0-255)
    // This would require additional implementation to store per-key colors
    // For now we'll just set the global color
    setRgbColor(r, g, b);
    break;

  case 0xC3:                     // Get current RGB color: /prefix/rgb/get
    Serial.write((uint8_t)0xC3); // Echo command as response
    Serial.write(ledR);
    Serial.write(ledG);
    Serial.write(ledB);
    Serial.write(globalBrightness);
    break;
  }
}
