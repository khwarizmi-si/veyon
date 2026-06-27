#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

binary_dir="$1"
install_files="$2"

cd "$binary_dir"

makensis "${install_files}/veyon.nsi"

installers=( "${install_files}"/veyon-*setup.exe )
if [ "${#installers[@]}" -eq 0 ]; then
	echo "ERROR: base installer was not produced by NSIS" >&2
	exit 1
fi

mv "${installers[@]}" .
rm -rf "${install_files}"
