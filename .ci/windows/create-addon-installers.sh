#!/usr/bin/env bash
set -euo pipefail

source_dir="$1"
binary_dir="$2"
install_files="$3"

cd "$binary_dir"

cp nsis/addon-chat.nsi "${install_files}"
cp nsis/addon-internetaccesscontrol.nsi "${install_files}"
cp nsis/addon-networkdiscovery.nsi "${install_files}"
cp nsis/addon-auvidus.nsi "${install_files}"
cp nsis/addon-screenrecorder.nsi "${install_files}"

pushd "${install_files}" > /dev/null

makensis addon-chat.nsi
makensis addon-internetaccesscontrol.nsi
makensis addon-networkdiscovery.nsi
makensis addon-auvidus.nsi

if [ -f "${source_dir}/3rdparty/ffmpeg/ffmpeg.exe" ]; then
	cp "${source_dir}/3rdparty/ffmpeg/ffmpeg.exe" .
	makensis addon-screenrecorder.nsi
else
	echo "SKIP ScreenRecorder installer: place an LGPL ffmpeg.exe at 3rdparty/ffmpeg/ffmpeg.exe"
fi

mv Sahid-*-Addon-*setup.exe "${binary_dir}"
