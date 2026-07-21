#include <stdint.h>

// I2C CONFIG

#undef I2C_SDA
#undef I2C_SCL

#define PICO 1
#define KB2040QT 2
#define FEATHER2040QT 3
#define DINKII 4
#define FRUITJAM 5

// DEFINE WHAT BOARD YOU ARE USING
// PICO OR KB2040QT or FEATHER2040QT or FRUITJAM
// if using feather, change platformio.ini board to use `adafruit_feather`
// if using fruit jam, change platformio.ini board to use `adafruit_fruitjam`
#ifndef BOARDTYPE
#define BOARDTYPE DINKII
#endif

// SET YOUR ADDRESSES
// 8x8 or 4x4 TEST
// const uint8_t addrRowOne[2] = {0x2F,0x2E};
// const uint8_t addrRowTwo[2] = {0x3E,0x36};

// 16x8
// neotrellis standard layout
// const uint8_t addrRowOne[4] = {0x32,0x30,0x2F,0x2E};
// const uint8_t addrRowTwo[4] = {0x33,0x31,0x3E,0x36};
// denki-oto mecha-layout
// const uint8_t addrRowOne[4] = {0x3E,0x36,0x2F,0x2E};
// const uint8_t addrRowTwo[4] = {0x33,0x31,0x32,0x30};

// for 16x16
const uint8_t addrRowOne[4] = {0x32, 0x30, 0x2F, 0x2E};
const uint8_t addrRowTwo[4] = {0x33, 0x31, 0x3E, 0x36};
const uint8_t addrRowThree[4] = {0x34, 0x38, 0x40, 0x3c};
const uint8_t addrRowFour[4] = {0x3a, 0x42, 0x4a, 0x46};

// DINKII - STEMMA-QT uses 2/3 and Wire1
#if BOARDTYPE == DINKII
#define MYWIRE Wire1
#define I2C_SDA 2
#define I2C_SCL 3
#endif

// KeeBoar KB2040 - STEMMA-QT uses 12/13 and Wire
#if BOARDTYPE == KB2040QT
#define MYWIRE Wire
#define I2C_SDA 12
#define I2C_SCL 13
#endif

// Feather RP2040 - STEMMA-QT uses 2/3 and Wire
#if BOARDTYPE == FEATHER2040QT
#define MYWIRE Wire
#define I2C_SDA 2
#define I2C_SCL 3
#endif

// Adafruit Fruit Jam RP2350 - STEMMA-QT uses 20/21 and Wire
#if BOARDTYPE == FRUITJAM
#define MYWIRE Wire
#define I2C_SDA 20
#define I2C_SCL 21
#endif

// DEFAULT FOR PICO
#ifndef I2C_SDA
#define I2C_SDA 26
#endif
#ifndef I2C_SCL
#define I2C_SCL 27
#endif
#ifndef MYWIRE
#define MYWIRE Wire1
#endif
