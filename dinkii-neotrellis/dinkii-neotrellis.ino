/***********************************************************
 *  DIY monome compatible grid w/ Adafruit NeoTrellis
 *  for RP2040 Pi Pico
 *
 *  This code makes the Adafruit Neotrellis boards into a Monome compatible grid via monome's mext protocol
 *  ----> https://www.adafruit.com/product/3954
 *
 *  Code here is for a 16x8 grid, but can be modified for 4x8, 8x8, or 16x16 (untested on larger grid arrays)
 *
 *  Many thanks to:
 *  scanner_darkly <https://github.com/scanner-darkly>,
 *  TheKitty <https://github.com/TheKitty>,
 *  Szymon Kaliski <https://github.com/szymonkaliski>,
 *  John Park, Todbot, Juanma, Gerald Stevens, and others
 *
 */

// SET TOOLS USB STACK TO TinyUSB

// RP2040 BOARDS REQUIRE Earle Philhower's Arduino core for RP2040 devices, arduino-pico
// See https://learn.adafruit.com/rp2040-arduino-with-the-earlephilhower-core/overview

// Be sure you have these libraries installed
//    Adafruit seesaw library
//    elapsedMillis
//    Adafruit TinyUSB Library
//    Adafruit NeoPixel

#include "MonomeSerialDevice.h"
#include <Adafruit_NeoTrellis.h>

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <elapsedMillis.h>

#include "i2c_config.h" // look here to change settings for different boards

#include "config.h" // look here to change settings for different boards

#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345, &MYWIRE);
bool accelAvailable = false;

bool isInited = false;
elapsedMillis monomeRefresh;

// Monome class setup
MonomeSerialDevice mdp;

uint32_t prevColorBuffer[mdp.MAXLEDCOUNT];

void sendLeds();

// NeoTrellis setup
#if GRIDCOUNT == SIXTEEN
// 1X1 TEST -- USES 8x8 address set
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[1], &MYWIRE)}};
#endif

#if GRIDCOUNT == SIXTYFOUR
// 8x8
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[0], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[1], &MYWIRE)},
    {Adafruit_NeoTrellis(addrRowTwo[0], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[1], &MYWIRE)}};
#endif

#if GRIDCOUNT == ONETWENTEIGHT
// 16x8
Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[0], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[1], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[2], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[3], &MYWIRE)}, // top row
    {Adafruit_NeoTrellis(addrRowTwo[0], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[1], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[2], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[3], &MYWIRE)}  // bottom row
};
#endif

#if GRIDCOUNT == TWOFIFTYSIX
// 16x16
const uint8_t trellis_addresses[NUM_ROWS / 4][NUM_COLS / 4] = {
    {addrRowOne[0], addrRowOne[1], addrRowOne[2], addrRowOne[3]},
    {addrRowTwo[0], addrRowTwo[1], addrRowTwo[2], addrRowTwo[3]},
    {addrRowThree[0], addrRowThree[1], addrRowThree[2], addrRowThree[3]},
    {addrRowFour[0], addrRowFour[1], addrRowFour[2], addrRowFour[3]}};

Adafruit_NeoTrellis trellis_array[NUM_ROWS / 4][NUM_COLS / 4] = {
    {Adafruit_NeoTrellis(addrRowOne[0], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[1], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[2], &MYWIRE), Adafruit_NeoTrellis(addrRowOne[3], &MYWIRE)},
    {Adafruit_NeoTrellis(addrRowTwo[0], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[1], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[2], &MYWIRE), Adafruit_NeoTrellis(addrRowTwo[3], &MYWIRE)},
    {Adafruit_NeoTrellis(addrRowThree[0], &MYWIRE), Adafruit_NeoTrellis(addrRowThree[1], &MYWIRE), Adafruit_NeoTrellis(addrRowThree[2], &MYWIRE), Adafruit_NeoTrellis(addrRowThree[3], &MYWIRE)},
    {Adafruit_NeoTrellis(addrRowFour[0], &MYWIRE), Adafruit_NeoTrellis(addrRowFour[1], &MYWIRE), Adafruit_NeoTrellis(addrRowFour[2], &MYWIRE), Adafruit_NeoTrellis(addrRowFour[3], &MYWIRE)}};
#endif

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array, NUM_ROWS / 4, NUM_COLS / 4);

#ifdef PANEL_DIAGNOSTICS
elapsedMillis panelDiagnosticTimer;
bool panelReachable[NUM_ROWS / 4][NUM_COLS / 4] = {};
uint8_t activeDiagnosticPanelRow = 0;
uint8_t activeDiagnosticPanelCol = 0;

const uint8_t PANEL_TEST_BRIGHTNESS = 48;
const uint32_t PANEL_TEST_IDLE_COLOR = 0x000040;
const uint32_t PANEL_TEST_PRESSED_COLOR = 0x00FF00;

TrellisCallback panelDiagnosticKeyCallback(keyEvent evt)
{
  if (evt.bit.EDGE != SEESAW_KEYPAD_EDGE_RISING &&
      evt.bit.EDGE != SEESAW_KEYPAD_EDGE_FALLING)
  {
    return 0;
  }

  const uint8_t key = evt.bit.NUM;
  const bool pressed = evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING;
  Adafruit_NeoTrellis &panel =
      trellis_array[activeDiagnosticPanelRow][activeDiagnosticPanelCol];

  panel.pixels.setPixelColor(
      key, pressed ? PANEL_TEST_PRESSED_COLOR : PANEL_TEST_IDLE_COLOR);
  panel.pixels.show();

  const uint8_t x = activeDiagnosticPanelCol * 4 + key % 4;
  const uint8_t y = activeDiagnosticPanelRow * 4 + key / 4;
  Serial.printf("Key (%u,%u) %s\n", x, y, pressed ? "PRESSED" : "released");
  return 0;
}

void configureDiagnosticPanel(uint8_t row, uint8_t col)
{
  Adafruit_NeoTrellis &panel = trellis_array[row][col];
  panel.pixels.setBrightness(PANEL_TEST_BRIGHTNESS);

  for (uint8_t key = 0; key < 16; key++)
  {
    panel.activateKey(key, SEESAW_KEYPAD_EDGE_RISING, true);
    panel.activateKey(key, SEESAW_KEYPAD_EDGE_FALLING, true);
    panel.registerCallback(key, panelDiagnosticKeyCallback);
    panel.pixels.setPixelColor(key, PANEL_TEST_IDLE_COLOR);
  }
  panel.pixels.show();
}

uint8_t probeTrellisPanels(bool initializePanels)
{
  uint8_t reachable = 0;
  uint8_t devices = 0;

  Serial.print("\nI2C addresses currently acknowledging:");
  for (uint8_t address = 1; address < 127; address++)
  {
    MYWIRE.beginTransmission(address);
    if (MYWIRE.endTransmission() == 0)
    {
      Serial.printf(" 0x%02X", address);
      devices++;
    }
  }
  Serial.printf(" (%u device%s)\n", devices, devices == 1 ? "" : "s");

  Serial.println("row col address status");
  for (uint8_t row = 0; row < NUM_ROWS / 4; row++)
  {
    for (uint8_t col = 0; col < NUM_COLS / 4; col++)
    {
      const uint8_t address = trellis_addresses[row][col];
      bool found;

      if (initializePanels || !panelReachable[row][col])
      {
        // begin() verifies that the device is a supported seesaw, not merely
        // that something acknowledges at this I2C address.
        found = trellis_array[row][col].begin(address);
      }
      else
      {
        MYWIRE.beginTransmission(address);
        found = MYWIRE.endTransmission() == 0;
      }

      Serial.printf(" %u   %u    0x%02X    %s\n", row + 1, col + 1, address,
                    found ? "OK" : "MISSING");

      if (found)
      {
        reachable++;
        if (initializePanels || !panelReachable[row][col])
        {
          configureDiagnosticPanel(row, col);
        }
      }
      panelReachable[row][col] = found;
    }
  }

  Serial.printf("Summary: %u/%u panels reachable\n", reachable,
                (NUM_ROWS / 4) * (NUM_COLS / 4));
  if (reachable != (NUM_ROWS / 4) * (NUM_COLS / 4))
  {
    Serial.println("Check power, solder joints, and address jumpers for every MISSING panel.");
  }
  else
  {
    Serial.println("All configured panels are reachable.");
  }

  return reachable;
}
#endif

// ***************************************************************************
// **                                HELPERS                                **
// ***************************************************************************

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85)
  {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

// ***************************************************************************
// **                          FUNCTIONS FOR TRELLIS                        **
// ***************************************************************************

// define a callback for key presses
TrellisCallback keyCallback(keyEvent evt)
{
  uint8_t x = evt.bit.NUM % NUM_COLS;
  uint8_t y = evt.bit.NUM / NUM_COLS;

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING)
  {
    //     Serial.println(" pressed ");
    mdp.sendGridKey(x, y, 1);
#if TEST
    trellis.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, NUM_ROWS, 0, 255))); // on rising
    trellis.show();
#endif
  }
  else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING)
  {
    //     Serial.println(" released ");
    mdp.sendGridKey(x, y, 0);
#if TEST
    trellis.setPixelColor(evt.bit.NUM, 0); // off falling
    trellis.show();
#endif
  }
  sendLeds();
  // trellis.show();
  return 0;
}

String serialNumberOne;
String serialNumberTwo;
// ***************************************************************************
// **                                 SETUP                                 **
// ***************************************************************************

void setup()
{
  uint8_t x, y;

  TinyUSBDevice.setManufacturerDescriptor(mfgstr);
#ifdef PANEL_DIAGNOSTICS
  TinyUSBDevice.setProductDescriptor("panel-test");
#else
  TinyUSBDevice.setProductDescriptor(prodstr);
#endif
  uint16_t tusb_serial[16];
  int validindices = TinyUSBDevice.getSerialDescriptor(tusb_serial);
  uint8_t serial_id[16] __attribute__((aligned(4)));
  uint8_t const serial_len = TinyUSB_Port_GetSerialNumber(serial_id);
  for (int i = 0; i < 8; i++)
  {
    serialNumberOne = String(serial_id[i], DEC);
    serialNumberTwo.concat(serialNumberOne);
  }
  String tempSerial = serialNumberTwo.substring(9);
  tempSerial.setCharAt(0, 'm');
  TinyUSBDevice.setSerialDescriptor(tempSerial.c_str());

  Serial.begin(115200);

  // while( !TinyUSBDevice.mounted() ) delay(1);

  MYWIRE.setSDA(I2C_SDA);
  MYWIRE.setSCL(I2C_SCL);
  MYWIRE.begin();
  MYWIRE.setClock(400000);

#ifdef PANEL_DIAGNOSTICS
  // wait a moment for serial monitor
  delay(2000);
  Serial.println("\n=== dinkii I2C diagnostics ===");
  Serial.printf("SDA=%d  SCL=%d\n", I2C_SDA, I2C_SCL);

  // I2C bus scan
  Serial.println("Scanning I2C bus...");
  int i2c_found = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    MYWIRE.beginTransmission(addr);
    if (MYWIRE.endTransmission() == 0)
    {
      Serial.printf("  Found device at 0x%02X\n", addr);
      i2c_found++;
    }
  }
  Serial.printf("Scan done: %d device(s)\n\n", i2c_found);
  Serial.println("=== 16x16 NeoTrellis/MechaTrellis panel test ===");
  Serial.println("All LEDs should be dim blue; pressed keys turn bright green.");
  probeTrellisPanels(true);
  panelDiagnosticTimer = 0;
  return;
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  mdp.isMonome = true;
  mdp.deviceID = deviceID;
  mdp.setupAsGrid(NUM_ROWS, NUM_COLS);
  mdp.setAllGridBaseColors(R, G, B);
  monomeRefresh = 0;
  isInited = true;

  int var = 0;
  while (var < 8)
  {
    mdp.poll();
    var++;
    delay(100);
  }

  // Keep USB/mext responsive and retry the whole array if a panel is
  // temporarily unavailable. This avoids the previous permanent failure loop.
  while (!trellis.begin())
  {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    const uint32_t retryStarted = millis();
    while (millis() - retryStarted < 500)
    {
      mdp.poll();
      delay(1);
    }
  }
  digitalWrite(LED_PIN, HIGH);

  // NeoPixels retain their last state across an RP2350 reset. Clear every
  // panel immediately, before the slower key and accelerometer setup, to
  // avoid a bright retained frame causing a startup brownout loop.
  for (x = 0; x < NUM_COLS / 4; x++)
  {
    for (y = 0; y < NUM_ROWS / 4; y++)
    {
      trellis_array[y][x].pixels.setBrightness(BRIGHTNESS);
      for (uint8_t pixel = 0; pixel < 16; pixel++)
      {
        trellis_array[y][x].pixels.setPixelColor(pixel, 0);
      }
      trellis_array[y][x].pixels.show();
    }
  }
  mdp.setAllLEDs(0);

  // ADXL343 Accelerometer init
  /* Initialise the sensor */
  accelAvailable = accel.begin();
  if (accelAvailable)
  {
    accel.setRange(ADXL343_RANGE_16_G); // 2/4/8/16 _G
    accel.setDataRate(ADXL343_DATARATE_100_HZ);
  }

  // key callback
  for (x = 0; x < NUM_COLS; x++)
  {
    for (y = 0; y < NUM_ROWS; y++)
    {
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, keyCallback);
    }
  }

  // blink one led to show it's started up
  trellis.setPixelColor(0, 0xFFFFFF);
  trellis.show();
  delay(100);
  trellis.setPixelColor(0, 0x000000);
  trellis.show();

#if TEST
  /* the array can be addressed as x,y or with the key number */
  for (int i = 0; i < NUM_LEDS; i++)
  {
    trellis.setPixelColor(i, Wheel(map(i, 0, NUM_COLS * NUM_ROWS, 0, 255))); // addressed with keynum
    trellis.show();
    delay(50);
  }
#endif
}

// ***************************************************************************
// **                                SEND LEDS                              **
// ***************************************************************************

void sendLeds()
{
  bool dirtyPanels[NUM_ROWS / 4][NUM_COLS / 4] = {};

  for (int i = 0; i < NUM_ROWS * NUM_COLS; i++)
  {
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    if (mdp.gridLedModes[i] == MonomeSerialDevice::GRID_LED_RGB)
    {
      red = mdp.gridRed[i];
      green = mdp.gridGreen[i];
      blue = mdp.gridBlue[i];
    }
    else
    {
      const uint8_t value = mdp.leds[i];
      const uint8_t scaled =
          mdp.gridLedModes[i] == MonomeSerialDevice::GRID_LED_LEVEL8
              ? value
              : min(255, gammaTable[value] * gammaAdj);
      red = ((uint16_t)scaled * mdp.gridRed[i]) / 255;
      green = ((uint16_t)scaled * mdp.gridGreen[i]) / 255;
      blue = ((uint16_t)scaled * mdp.gridBlue[i]) / 255;
    }

    red = ((uint16_t)red * mdp.gridIntensity) / 255;
    green = ((uint16_t)green * mdp.gridIntensity) / 255;
    blue = ((uint16_t)blue * mdp.gridIntensity) / 255;
    const uint32_t hexColor =
        ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;

    if (hexColor != prevColorBuffer[i])
    {
      trellis.setPixelColor(i, hexColor);

      prevColorBuffer[i] = hexColor;
      const uint8_t x = i % NUM_COLS;
      const uint8_t y = i / NUM_COLS;
      dirtyPanels[y / 4][x / 4] = true;
    }
  }
  for (uint8_t row = 0; row < NUM_ROWS / 4; row++)
  {
    for (uint8_t col = 0; col < NUM_COLS / 4; col++)
    {
      if (dirtyPanels[row][col])
      {
        trellis_array[row][col].pixels.show();
      }
    }
  }
}

// ***************************************************************************
// **                                 LOOP                                  **
// ***************************************************************************

void loop()
{
#ifdef PANEL_DIAGNOSTICS
  for (uint8_t row = 0; row < NUM_ROWS / 4; row++)
  {
    for (uint8_t col = 0; col < NUM_COLS / 4; col++)
    {
      if (panelReachable[row][col])
      {
        activeDiagnosticPanelRow = row;
        activeDiagnosticPanelCol = col;
        trellis_array[row][col].read();
      }
    }
  }

  if (panelDiagnosticTimer >= 3000)
  {
    probeTrellisPanels(false);
    panelDiagnosticTimer = 0;
  }
  delay(5);
  return;
#endif

  mdp.poll(); // process incoming serial from Monomes

  // refresh every 16ms or so
  if (isInited && monomeRefresh > 16)
  {
    if (accelAvailable && mdp.tiltState[0])
    {
      sensors_event_t event;
      accel.getEvent(&event);
      int16_t axis[3];
      axis[0] = (event.acceleration.x * 2) + 128;
      axis[1] = (event.acceleration.y * 2) + 128;
      axis[2] = (event.acceleration.z * 2) + 128;
      int8_t *axisbytes = (int8_t *)axis;
      mdp.sendTiltEvent(0, axisbytes[0], axisbytes[1], axisbytes[2], axisbytes[3], axisbytes[4], axisbytes[5]);
    }
    trellis.read();
    sendLeds();
    monomeRefresh = 0;
  }

  //     // Send tilt data every 100ms
  //     if (currentMillis - lastTiltCheck >= 100) {
  //         lastTiltCheck = currentMillis;
  //         sendTiltData();
  //     }
}

// void sendTiltData() {
//     sensors_event_t a, g, temp;
//     mpu.getEvent(&a, &g, &temp);
//
//     // Scale gyro data to 16-bit integer range
//     int16_t x = (int16_t)(g.gyro.x * 1000);
//     int16_t y = (int16_t)(g.gyro.y * 1000);
//     int16_t z = (int16_t)(g.gyro.z * 1000);
//
//     mdp.sendTiltEvent(0, x, y, z);
// }
