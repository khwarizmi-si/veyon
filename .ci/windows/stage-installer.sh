#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

source_dir="$1"
binary_dir="$2"
install_files="$3"
dll_dir="$4"
dll_dir_lib="$5"
dll_dir_gcc="$6"
dll_gcc="$7"
dll_ddengine="$8"
mingw_prefix="$9"
strip_tool="${10}"

cd "$binary_dir"

copy_required()
{
	local target_dir="$1"
	shift

	local matches=()
	local pattern
	for pattern in "$@"; do
		matches+=( ${pattern} )
	done

	if [ "${#matches[@]}" -eq 0 ]; then
		echo "ERROR: required file pattern not found: $*" >&2
		return 1
	fi

	cp "${matches[@]}" "${target_dir}"
}

copy_optional()
{
	local target_dir="$1"
	shift

	local matches=()
	local pattern
	for pattern in "$@"; do
		matches+=( ${pattern} )
	done

	if [ "${#matches[@]}" -gt 0 ]; then
		cp "${matches[@]}" "${target_dir}"
	else
		echo "WARNING: optional file pattern not found: $*"
	fi
}

find_required_file()
{
	local pattern="$1"
	local match

	match="$(find "${mingw_prefix}" -name "${pattern}" -print -quit)"
	if [ -z "${match}" ]; then
		echo "ERROR: required ${pattern} not found under ${mingw_prefix}" >&2
		return 1
	fi

	printf '%s\n' "${match}"
}

copy_found_required()
{
	local target_dir="$1"
	local pattern="$2"
	local match

	match="$(find_required_file "${pattern}")"
	cp "${match}" "${target_dir}"
}

qt_plugin_dir()
{
	local relative_dir="$1"
	local candidate

	for candidate in "${mingw_prefix}/plugins/${relative_dir}" "${mingw_prefix}/share/qt6/plugins/${relative_dir}"; do
		if [ -d "${candidate}" ]; then
			printf '%s\n' "${candidate}"
			return 0
		fi
	done

	find "${mingw_prefix}" -type d -path "*/plugins/${relative_dir}" -print -quit
}

copy_qt_plugin_optional()
{
	local relative_dir="$1"
	local plugin_name="$2"
	local target_dir="${install_files}/${relative_dir}"
	local plugin_dir

	mkdir -p "${target_dir}"
	plugin_dir="$(qt_plugin_dir "${relative_dir}")"

	if [ -n "${plugin_dir}" ] && [ -f "${plugin_dir}/${plugin_name}" ]; then
		cp "${plugin_dir}/${plugin_name}" "${target_dir}"
	else
		echo "WARNING: optional Qt plugin ${relative_dir}/${plugin_name} not found"
	fi
}

copy_qt_plugin_required()
{
	local relative_dir="$1"
	local plugin_name="$2"
	local target_dir="${install_files}/${relative_dir}"
	local plugin_dir

	mkdir -p "${target_dir}"
	plugin_dir="$(qt_plugin_dir "${relative_dir}")"

	if [ -n "${plugin_dir}" ] && [ -f "${plugin_dir}/${plugin_name}" ]; then
		cp "${plugin_dir}/${plugin_name}" "${target_dir}"
	else
		echo "ERROR: required Qt plugin ${relative_dir}/${plugin_name} not found" >&2
		return 1
	fi
}

rm -rf "${install_files}"*

mkdir -p "${install_files}/interception"
cp "${source_dir}/3rdparty/interception/"* "${install_files}/interception"
cp "${source_dir}/3rdparty/ddengine/${dll_ddengine}" "${install_files}"
cp core/veyon-core.dll "${install_files}"

find cli configurator master server service worker -name 'veyon-*.exe' -exec cp '{}' "${install_files}/" ';'

mkdir -p "${install_files}/plugins"
find plugins/ -name '*.dll' -exec cp '{}' "${install_files}/plugins/" ';'

if compgen -G "${install_files}/plugins/lib*.dll" > /dev/null; then
	mv "${install_files}"/plugins/lib*.dll "${install_files}"
fi

if [ -f "${install_files}/plugins/vnchooks.dll" ]; then
	mv "${install_files}/plugins/vnchooks.dll" "${install_files}"
fi

mkdir -p "${install_files}/translations"
cp translations/*qm "${install_files}/translations/"

copy_optional "${install_files}" "${dll_dir}"/libjpeg*.dll
copy_required "${install_files}" "${dll_dir}"/libpng*.dll
copy_required "${install_files}" "${dll_dir}"/libcrypto-3*.dll "${dll_dir}"/libssl-3*.dll
copy_found_required "${install_files}" libqca-qt6.dll
copy_required "${install_files}" "${dll_dir}"/libsasl2*.dll
copy_required "${install_files}" "${dll_dir}"/libldap*.dll "${dll_dir}"/liblber*.dll
copy_required "${install_files}" "${dll_dir}/interception.dll"
copy_required "${install_files}" "${dll_dir}"/liblzo2*.dll
copy_required "${install_files}" "${dll_dir}/libvncclient.dll"
copy_required "${install_files}" "${dll_dir}/libvncserver.dll"
copy_found_required "${install_files}" zlib1.dll
copy_found_required "${install_files}" libwinpthread-1.dll
copy_found_required "${install_files}" libstdc++-6.dll

if [ -f "${dll_dir_gcc}/libssp-0.dll" ]; then
	cp "${dll_dir_gcc}/libssp-0.dll" "${install_files}"
fi

copy_found_required "${install_files}" "${dll_gcc}"

mkdir -p "${install_files}/crypto"
qca_ossl_plugin="$(find "${mingw_prefix}" -path '*/crypto/libqca-ossl.dll' -print -quit)"
if [ -n "${qca_ossl_plugin}" ]; then
	cp "${qca_ossl_plugin}" "${install_files}/crypto"
else
	echo "WARNING: libqca-ossl.dll not found under ${mingw_prefix}; continuing without QCA OpenSSL plugin"
fi

for qt_dll in Qt6Core.dll Qt6Core5Compat.dll Qt6Gui.dll Qt6Widgets.dll Qt6Network.dll Qt6Concurrent.dll Qt6HttpServer.dll Qt6WebSockets.dll; do
	copy_found_required "${install_files}" "${qt_dll}"
done

copy_qt_plugin_optional imageformats qjpeg.dll
copy_qt_plugin_required platforms qwindows.dll
copy_qt_plugin_optional tls qopensslbackend.dll

styles_dir="$(qt_plugin_dir styles)"
mkdir -p "${install_files}/styles"
if [ -n "${styles_dir}" ]; then
	copy_optional "${install_files}/styles" "${styles_dir}"/*.dll
else
	echo "WARNING: optional Qt styles plugin directory not found"
fi

strip_targets=(
	"${install_files}"/*.dll
	"${install_files}"/*.exe
	"${install_files}"/plugins/*.dll
	"${install_files}"/platforms/*.dll
	"${install_files}"/styles/*.dll
	"${install_files}"/crypto/*.dll
)

if [ "${#strip_targets[@]}" -gt 0 ]; then
	"${strip_tool}" "${strip_targets[@]}"
fi

cp "${source_dir}/COPYING" "${install_files}"
cp "${source_dir}/COPYING" "${install_files}/LICENSE.TXT"
cp "${source_dir}/README.md" "${install_files}/README.TXT"
todos "${install_files}"/*.TXT

cp -ra "${source_dir}/nsis" "${install_files}"
cp "${binary_dir}/nsis/veyon.nsi" "${install_files}"

find "${install_files}" -ls
