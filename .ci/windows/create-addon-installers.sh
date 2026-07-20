#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

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

staging_dir="$(cygpath -w "$(pwd)")"
export STAGING_DIR="${staging_dir}"
perl -0pi -e 's{(!define ADDON_DLL "([^"]+)")}{qq{$1\n!define ADDON_DLL_PATH "$ENV{STAGING_DIR}\\plugins\\$2"}}ge;' addon-*.nsi

build_addon()
{
	local script="$1"
	local dll="$2"

	if [ -f "plugins/${dll}" ]; then
		makensis "${script}"
	else
		echo "SKIP ${script}: plugins/${dll} was not built for this branch"
	fi
}

build_addon addon-chat.nsi chat.dll
build_addon addon-internetaccesscontrol.nsi internetaccesscontrol.dll
build_addon addon-networkdiscovery.nsi networkdiscovery.dll
build_addon addon-auvidus.nsi auvidus.dll

if [ -f "plugins/screenrecorder.dll" ] && [ -f "${source_dir}/3rdparty/ffmpeg/ffmpeg.exe" ]; then
	cp "${source_dir}/3rdparty/ffmpeg/ffmpeg.exe" .
	perl -0pi -e 's{!define ADDON_EXTRA_FILE "ffmpeg\.exe"}{qq{!define ADDON_EXTRA_FILE "$ENV{STAGING_DIR}\\ffmpeg.exe"}}ge;' addon-screenrecorder.nsi
	makensis addon-screenrecorder.nsi
elif [ ! -f "plugins/screenrecorder.dll" ]; then
	echo "SKIP addon-screenrecorder.nsi: plugins/screenrecorder.dll was not built for this branch"
else
	echo "SKIP ScreenRecorder installer: place an LGPL ffmpeg.exe at 3rdparty/ffmpeg/ffmpeg.exe"
fi

addons=( Sahid-*-Addon-*setup.exe )
if [ "${#addons[@]}" -gt 0 ]; then
	mv "${addons[@]}" "${binary_dir}"
else
	echo "SKIP add-on artifact move: no add-on installers were produced"
fi

popd > /dev/null
