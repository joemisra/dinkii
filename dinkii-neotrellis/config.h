#include <stdint.h>
#include <Arduino.h>

#define TEST 0 // SET TO 1 for testing

#define SIXTEEN 1
#define SIXTYFOUR 2
#define ONETWENTEIGHT 3
#define TWOFIFTYSIX 4

// Which Grid - SIXTEEN, SIXTYFOUR, ONETWENTEIGHT, TWOFIFTYSIX
#ifndef GRIDCOUNT
#define GRIDCOUNT ONETWENTEIGHT
#endif

#if GRIDCOUNT == SIXTEEN
#define NUM_ROWS 4 // down - rows
#define NUM_COLS 4 // across - columns
#endif
#if GRIDCOUNT == SIXTYFOUR
#define NUM_ROWS 8 // down - rows
#define NUM_COLS 8 // across - columns
#endif
#if GRIDCOUNT == ONETWENTEIGHT
#define NUM_ROWS 8  // down - rows
#define NUM_COLS 16 // across - columns
#endif
#if GRIDCOUNT == TWOFIFTYSIX
#define NUM_ROWS 16 // down - rows
#define NUM_COLS 16 // across - columns
#endif

#define NUM_LEDS NUM_ROWS *NUM_COLS

#define INT_PIN 9
// #define LED_PIN 13 // teensy LED used to show boot info
#define LED_PIN 16  // dinkii LED1
#define LED_PIN2 18 // dinkii LED2

// This assumes you are using a USB breakout board to route power to the board
// If you are plugging directly into the controller, you will need to adjust this brightness to a much lower value
// These values can be overridden at runtime via CONFIG.TXT on the USB drive
uint8_t ledBrightness = 127; // overall grid brightness (0-255)
uint8_t ledR = 255;          // LED red channel (0-255)
uint8_t ledG = 255;          // LED green channel (0-255)
uint8_t ledB = 255;          // LED blue channel (0-255)
uint8_t ledGammaAdj = 2;     // gamma adjustment (1 or 2)
uint8_t rowStart = 0;        // row offset (0 = top half, 8 = bottom half of a 256)

// gamma table for 16 levels of brightness
const uint8_t gammaTable[16] = {0, 2, 3, 6, 11, 18, 25, 32, 41, 59, 70, 80, 92, 103, 115, 127};

// set your monome device name here (can also be set in CONFIG.TXT)
String deviceID = "monome";
String serialNum = "m4216126";

// DEVICE INFO FOR TinyUSB
char mfgstr[32] = "monome";
char prodstr[32] = "grid";
char serialstr[32] = "m4216126";
