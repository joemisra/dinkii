#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This activation helper is only for macOS." >&2
  exit 1
fi

host_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
serialoscd="${host_dir}/dist/bin/serialoscd"
label="coffee.dsp.mechatrellis-serialosc"
domain="gui/$(id -u)"
launch_agents="${HOME}/Library/LaunchAgents"
plist="${launch_agents}/${label}.plist"
log_dir="${HOME}/Library/Logs"
log_file="${log_dir}/mechatrellis-serialosc.log"

if [[ ! -x "${serialoscd}" ]]; then
  echo "Custom serialoscd is missing; run ./host/build_serialosc.sh first." >&2
  exit 1
fi

mkdir -p "${launch_agents}" "${log_dir}"

if command -v brew >/dev/null 2>&1; then
  brew services stop serialosc || true
fi

launchctl bootout "${domain}/${label}" >/dev/null 2>&1 || true

plutil -create xml1 "${plist}"
plutil -insert Label -string "${label}" "${plist}"
plutil -insert ProgramArguments -array "${plist}"
plutil -insert ProgramArguments.0 -string "${serialoscd}" "${plist}"
plutil -insert RunAtLoad -bool true "${plist}"
plutil -insert KeepAlive -bool true "${plist}"
plutil -insert StandardOutPath -string "${log_file}" "${plist}"
plutil -insert StandardErrorPath -string "${log_file}" "${plist}"

launchctl bootstrap "${domain}" "${plist}"

echo "Activated the MechaTrellis serialosc service."
echo "LaunchAgent: ${plist}"
echo "Log: ${log_file}"
