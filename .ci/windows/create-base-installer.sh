#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

binary_dir="$1"
install_files="$2"

cd "$binary_dir"

pushd "${install_files}" > /dev/null
makensis veyon.nsi

installers=( veyon-*setup.exe )
if [ "${#installers[@]}" -eq 0 ]; then
	echo "ERROR: base installer was not produced by NSIS" >&2
	exit 1
fi

mv "${installers[@]}" "${binary_dir}"
popd > /dev/null
rm -rf "${install_files}"
