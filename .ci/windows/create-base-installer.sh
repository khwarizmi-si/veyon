#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

binary_dir="$1"
install_files="$2"

cd "$binary_dir"

mkdir -p nsis
cp -ra "${install_files}/nsis/." "${binary_dir}/nsis/"

for asset in nsis/welcome-page.bmp nsis/header.bmp nsis/installer.ico nsis/uninstaller.ico; do
	if [ ! -f "${asset}" ]; then
		echo "ERROR: required NSIS asset is missing: ${asset}" >&2
		exit 1
	fi
done

makensis "${install_files}/veyon.nsi"

installers=( "${install_files}"/veyon-*setup.exe veyon-*setup.exe )
if [ "${#installers[@]}" -eq 0 ]; then
	echo "ERROR: base installer was not produced by NSIS" >&2
	exit 1
fi

mv "${installers[@]}" "${binary_dir}"
rm -rf "${install_files}"
