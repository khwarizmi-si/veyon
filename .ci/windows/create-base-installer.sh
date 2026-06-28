#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

binary_dir="$1"
install_files="$2"

cd "$binary_dir"

for asset in "${install_files}"/nsis/welcome-page.bmp "${install_files}"/nsis/header.bmp "${install_files}"/nsis/installer.ico "${install_files}"/nsis/uninstaller.ico; do
	if [ ! -f "${asset}" ]; then
		echo "ERROR: required NSIS asset is missing: ${asset}" >&2
		exit 1
	fi
done

pushd "${install_files}" > /dev/null

nsis_dir="$(cygpath -w "$(pwd)/nsis")"
export NSIS_DIR="${nsis_dir}"
perl -0pi -e 's{"nsis/welcome-page\.bmp"}{qq{"$ENV{NSIS_DIR}\\welcome-page.bmp"}}ge;
              s{"nsis/header\.bmp"}{qq{"$ENV{NSIS_DIR}\\header.bmp"}}ge;
              s{"nsis/installer\.ico"}{qq{"$ENV{NSIS_DIR}\\installer.ico"}}ge;
              s{"nsis/uninstaller\.ico"}{qq{"$ENV{NSIS_DIR}\\uninstaller.ico"}}ge;' veyon.nsi

makensis veyon.nsi

popd > /dev/null

installers=( "${install_files}"/veyon-*setup.exe veyon-*setup.exe )
if [ "${#installers[@]}" -eq 0 ]; then
	echo "ERROR: base installer was not produced by NSIS" >&2
	exit 1
fi

mv "${installers[@]}" "${binary_dir}"
rm -rf "${install_files}"
