#!/usr/bin/env bash
set -euo pipefail

host_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
output_dir="${1:-${host_dir}/dist}"
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/mechatrellis-serialosc.XXXXXX")"

cleanup() {
  case "${work_dir}" in
    "${TMPDIR:-/tmp}"/mechatrellis-serialosc.*) rm -rf "${work_dir}" ;;
  esac
}
trap cleanup EXIT

git clone --quiet --depth 1 --branch v1.4.10 \
  https://github.com/monome/libmonome.git "${work_dir}/libmonome"
git -C "${work_dir}/libmonome" apply \
  "${host_dir}/patches/libmonome-v1.4.10-mechatrellis.patch"

git clone --quiet --depth 1 --branch v1.4.7 \
  https://github.com/monome/serialosc.git "${work_dir}/serialosc"
git -C "${work_dir}/serialosc" submodule update --init --recursive --depth 1
git -C "${work_dir}/serialosc" apply \
  "${host_dir}/patches/serialosc-v1.4.7-mechatrellis.patch"

cmake -S "${work_dir}/serialosc" -B "${work_dir}/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${output_dir}" \
  -DMECHATRELLIS_LIBMONOME_DIR="${work_dir}/libmonome"
cmake --build "${work_dir}/build" --parallel
cmake -E make_directory "${output_dir}/bin"
for executable in serialosc-device serialosc-detector serialoscd; do
  cmake -E copy_if_different \
    "${work_dir}/build/bin/${executable}" "${output_dir}/bin/${executable}"
done

echo "MechaTrellis serialosc installed in ${output_dir}"
echo "This does not replace or restart the Homebrew serialosc service."
