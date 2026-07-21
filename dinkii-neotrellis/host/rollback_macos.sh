#!/usr/bin/env bash
set -euo pipefail

label="coffee.dsp.mechatrellis-serialosc"
domain="gui/$(id -u)"
plist="${HOME}/Library/LaunchAgents/${label}.plist"

launchctl bootout "${domain}/${label}" >/dev/null 2>&1 || true

if command -v brew >/dev/null 2>&1; then
  brew services start serialosc
  echo "Restored the Homebrew serialosc service."
else
  echo "Homebrew was not found; start the stock serialosc service manually." >&2
fi

echo "The inactive custom LaunchAgent remains at ${plist}."
