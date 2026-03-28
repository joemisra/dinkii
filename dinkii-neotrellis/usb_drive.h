#ifndef USB_DRIVE_H
#define USB_DRIVE_H

#include <Arduino.h>

// Initialize USB mass storage drive backed by RP2040 internal flash.
// Reads CONFIG.TXT from the FAT12 filesystem (auto-formats on first boot).
// Must be called BEFORE Serial.begin() in setup().
void usbDriveSetup();

// Returns true if the host PC wrote to the drive since the last check.
bool usbDriveChanged();

// Re-read CONFIG.TXT and apply values to runtime config variables.
void usbDriveReloadConfig();

#endif
