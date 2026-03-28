#include "usb_drive.h"
#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

// Runtime config variables defined in config.h (linked via .ino translation unit)
extern uint8_t ledBrightness;
extern uint8_t ledR, ledG, ledB;
extern uint8_t ledGammaAdj;
extern uint8_t rowStart;
extern String deviceID;

// RP2040 internal flash transport.
// Default constructor uses the partition configured by board_build.filesystem_size.
Adafruit_FlashTransport_RP2040 flashTransport;
Adafruit_SPIFlash flash(&flashTransport);

FatVolume fatfs;
Adafruit_USBD_MSC usb_msc;

static volatile bool _fs_changed = false;
static bool _fs_formatted = false;

// ---------------------------------------------------------------------------
//  MSC callbacks (called by TinyUSB when the host reads/writes the drive)
// ---------------------------------------------------------------------------

static int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize) {
  return flash.readBlocks(lba, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
}

static int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize) {
  return flash.writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

static void msc_flush_cb(void) {
  flash.syncBlocks();
  fatfs.cacheClear();
  _fs_changed = true;
}

// ---------------------------------------------------------------------------
//  Minimal FAT12 formatter for first boot
// ---------------------------------------------------------------------------

static bool formatFAT12() {
  uint32_t sectorCount = flash.size() / 512;
  if (sectorCount == 0) return false;

  uint8_t sectorsPerCluster = 1;
  if (sectorCount > 4096) sectorsPerCluster = 4;

  uint16_t reservedSectors = 1;
  uint8_t numFATs = 1;
  uint16_t rootEntryCount = 32;
  uint16_t rootDirSectors = ((rootEntryCount * 32) + 511) / 512;

  // Iteratively solve for sectors-per-FAT (FAT12: 12 bits per cluster entry)
  uint16_t sectorsPerFAT = 1;
  for (int i = 0; i < 16; i++) {
    uint32_t dataSectors = sectorCount - reservedSectors
                           - rootDirSectors - (numFATs * sectorsPerFAT);
    uint32_t clusters    = dataSectors / sectorsPerCluster;
    uint32_t fatBytes     = ((clusters + 2) * 3 + 1) / 2;
    uint16_t needed       = (fatBytes + 511) / 512;
    if (needed <= sectorsPerFAT) break;
    sectorsPerFAT = needed;
  }

  uint8_t buf[512];

  // --- Boot sector (BPB) ---
  memset(buf, 0, sizeof(buf));
  buf[0] = 0xEB; buf[1] = 0x3C; buf[2] = 0x90;          // JMP short
  memcpy(buf + 3, "MSDOS5.0", 8);                         // OEM
  buf[11] = 0x00; buf[12] = 0x02;                         // bytes/sector = 512
  buf[13] = sectorsPerCluster;
  buf[14] = reservedSectors & 0xFF;
  buf[15] = reservedSectors >> 8;
  buf[16] = numFATs;
  buf[17] = rootEntryCount & 0xFF;
  buf[18] = rootEntryCount >> 8;
  if (sectorCount <= 0xFFFF) {
    buf[19] = sectorCount & 0xFF;
    buf[20] = (sectorCount >> 8) & 0xFF;
  } else {
    buf[32] = sectorCount & 0xFF;
    buf[33] = (sectorCount >> 8) & 0xFF;
    buf[34] = (sectorCount >> 16) & 0xFF;
    buf[35] = (sectorCount >> 24) & 0xFF;
  }
  buf[21] = 0xF8;                                         // media type
  buf[22] = sectorsPerFAT & 0xFF;
  buf[23] = sectorsPerFAT >> 8;
  buf[24] = 1; buf[25] = 0;                               // sectors/track
  buf[26] = 1; buf[27] = 0;                               // heads
  buf[36] = 0x80;                                          // drive number
  buf[38] = 0x29;                                          // ext boot sig
  buf[39] = 0x44; buf[40] = 0x4E; buf[41] = 0x4B; buf[42] = 0x32; // serial "DNK2"
  memcpy(buf + 43, "DINKII     ", 11);                     // volume label
  memcpy(buf + 54, "FAT12   ", 8);                         // FS type
  buf[510] = 0x55; buf[511] = 0xAA;                        // boot signature
  if (!flash.writeBlocks(0, buf, 1)) return false;

  // --- FAT table ---
  memset(buf, 0, sizeof(buf));
  buf[0] = 0xF8; buf[1] = 0xFF; buf[2] = 0xFF;           // reserved entries 0 & 1
  if (!flash.writeBlocks(reservedSectors, buf, 1)) return false;

  memset(buf, 0, sizeof(buf));
  for (uint16_t i = 1; i < sectorsPerFAT; i++) {
    if (!flash.writeBlocks(reservedSectors + i, buf, 1)) return false;
  }

  // --- Root directory with volume label ---
  memset(buf, 0, sizeof(buf));
  memcpy(buf, "DINKII     ", 11);                          // label (8.3 padded)
  buf[11] = 0x08;                                          // attribute: volume label
  for (uint16_t i = 0; i < rootDirSectors; i++) {
    if (!flash.writeBlocks(reservedSectors + sectorsPerFAT + i,
                           (i == 0) ? buf : (uint8_t *)memset(buf, 0, sizeof(buf)),
                           1))
      return false;
  }

  flash.syncBlocks();
  return true;
}

// ---------------------------------------------------------------------------
//  Config file parser
// ---------------------------------------------------------------------------

static void parseConfigLine(const char *line) {
  while (*line == ' ' || *line == '\t') line++;
  if (*line == '#' || *line == '\0' || *line == '\n' || *line == '\r') return;

  const char *eq = strchr(line, '=');
  if (!eq) return;

  char key[32];
  int keyLen = eq - line;
  if (keyLen >= (int)sizeof(key)) keyLen = sizeof(key) - 1;
  memcpy(key, line, keyLen);
  key[keyLen] = '\0';
  while (keyLen > 0 && (key[keyLen - 1] == ' ' || key[keyLen - 1] == '\t'))
    key[--keyLen] = '\0';

  const char *val = eq + 1;
  while (*val == ' ' || *val == '\t') val++;
  char value[64];
  strncpy(value, val, sizeof(value) - 1);
  value[sizeof(value) - 1] = '\0';
  int valLen = strlen(value);
  while (valLen > 0 &&
         (value[valLen - 1] == ' ' || value[valLen - 1] == '\t' ||
          value[valLen - 1] == '\r' || value[valLen - 1] == '\n'))
    value[--valLen] = '\0';

  if (strcmp(key, "brightness") == 0)    { ledBrightness = constrain(atoi(value), 0, 255); }
  else if (strcmp(key, "r") == 0)        { ledR          = constrain(atoi(value), 0, 255); }
  else if (strcmp(key, "g") == 0)        { ledG          = constrain(atoi(value), 0, 255); }
  else if (strcmp(key, "b") == 0)        { ledB          = constrain(atoi(value), 0, 255); }
  else if (strcmp(key, "gamma") == 0)    { ledGammaAdj   = constrain(atoi(value), 1, 2);  }
  else if (strcmp(key, "row_start") == 0){ rowStart      = constrain(atoi(value), 0, 24); }
  else if (strcmp(key, "device_id") == 0){ deviceID = value; }
}

static bool readConfig() {
  if (!_fs_formatted) return false;

  FatFile file;
  if (!file.open("CONFIG.TXT", O_RDONLY)) return false;

  char line[128];
  int pos = 0;
  int c;
  while ((c = file.read()) >= 0) {
    if (c == '\n' || pos >= (int)sizeof(line) - 1) {
      line[pos] = '\0';
      parseConfigLine(line);
      pos = 0;
    } else {
      line[pos++] = (char)c;
    }
  }
  if (pos > 0) {
    line[pos] = '\0';
    parseConfigLine(line);
  }
  file.close();
  return true;
}

static String generateDefaultConfig() {
  String cfg;
  cfg += "# ================================\n";
  cfg += "# dinkii grid configuration\n";
  cfg += "# ================================\n";
  cfg += "# Edit values below, then reset\n";
  cfg += "# the device to apply changes.\n";
  cfg += "#\n";
  cfg += "# Grid size is set at compile time\n";
  cfg += "# and cannot be changed here.\n";
  cfg += "\n";
  cfg += "# LED brightness (0-255)\n";
  cfg += "brightness=" + String(ledBrightness) + "\n";
  cfg += "\n";
  cfg += "# LED color (0-255 each)\n";
  cfg += "r=" + String(ledR) + "\n";
  cfg += "g=" + String(ledG) + "\n";
  cfg += "b=" + String(ledB) + "\n";
  cfg += "\n";
  cfg += "# Gamma adjustment (1 or 2)\n";
  cfg += "gamma=" + String(ledGammaAdj) + "\n";
  cfg += "\n";
  cfg += "# Row offset for 256 grids (0 = top, 8 = bottom)\n";
  cfg += "row_start=" + String(rowStart) + "\n";
  cfg += "\n";
  cfg += "# Device name (monome identifier)\n";
  cfg += "device_id=" + deviceID + "\n";
  return cfg;
}

static bool createDefaultConfig() {
  if (!_fs_formatted) return false;

  FatFile file;
  if (!file.open("CONFIG.TXT", O_WRONLY | O_CREAT | O_TRUNC)) return false;

  String cfg = generateDefaultConfig();
  file.write(cfg.c_str(), cfg.length());
  file.close();
  flash.syncBlocks();
  return true;
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------

void usbDriveSetup() {
  flash.begin();

  // Register MSC interface (must happen before TinyUSBDevice starts)
  usb_msc.setID("dinkii", "Config Drive", "1.0");
  usb_msc.setCapacity(flash.size() / 512, 512);
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(true);
  usb_msc.begin();

  // Mount or format filesystem
  _fs_formatted = fatfs.begin(&flash);
  if (!_fs_formatted) {
    if (formatFAT12()) {
      _fs_formatted = fatfs.begin(&flash);
    }
  }

  if (_fs_formatted) {
    FatFile test;
    if (!test.open("CONFIG.TXT", O_RDONLY)) {
      createDefaultConfig();
    } else {
      test.close();
    }
    readConfig();
  }
}

bool usbDriveChanged() {
  if (_fs_changed) {
    _fs_changed = false;
    return true;
  }
  return false;
}

void usbDriveReloadConfig() {
  if (!_fs_formatted) {
    _fs_formatted = fatfs.begin(&flash);
  }
  if (_fs_formatted) {
    fatfs.cacheClear();
    readConfig();
  }
}
