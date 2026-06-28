#!/usr/bin/env bash
set -euo pipefail

source_dir="$1"
binary_dir="$2"
install_files="$3"

cd "$binary_dir"

cp -ra "${install_files}/nsis/." "${binary_dir}/nsis/"
cp nsis/addon-chat.nsi "${install_files}"
cp nsis/addon-internetaccesscontrol.nsi "${install_files}"
cp nsis/addon-networkdiscovery.nsi "${install_files}"
cp nsis/addon-auvidus.nsi "${install_files}"
cp nsis/addon-screenrecorder.nsi "${install_files}"

makensis "${install_files}/addon-chat.nsi"
makensis "${install_files}/addon-internetaccesscontrol.nsi"
makensis "${install_files}/addon-networkdiscovery.nsi"
makensis "${install_files}/addon-auvidus.nsi"

if [ -f "${source_dir}/3rdparty/ffmpeg/ffmpeg.exe" ]; then
	cp "${source_dir}/3rdparty/ffmpeg/ffmpeg.exe" "${install_files}/"
	makensis "${install_files}/addon-screenrecorder.nsi"
else
	echo "SKIP ScreenRecorder installer: place an LGPL ffmpeg.exe at 3rdparty/ffmpeg/ffmpeg.exe"
fi

mv "${install_files}"/Sahid-*-Addon-*setup.exe "${binary_dir}"
