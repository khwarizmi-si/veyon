#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

binary_dir="$1"
install_files="$2"

cd "$binary_dir"

cp -ra "${install_files}/nsis/." "${binary_dir}/nsis/"
nsis_dir="$(cygpath -m "${binary_dir}/nsis")"
sed -i \
	-e "s#\"nsis/#\"${nsis_dir}/#g" \
	-e "s#\"nsis\\\\#\"${nsis_dir}/#g" \
	"${install_files}/veyon.nsi"
makensis "${install_files}/veyon.nsi"

installers=( "${install_files}"/veyon-*setup.exe )
if [ "${#installers[@]}" -eq 0 ]; then
	echo "ERROR: base installer was not produced by NSIS" >&2
	exit 1
fi

mv "${installers[@]}" "${binary_dir}"
rm -rf "${install_files}"
