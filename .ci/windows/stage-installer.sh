#!/usr/bin/env bash
set -euo pipefail

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

cp "${dll_dir}"/libjpeg*.dll "${install_files}"
cp "${dll_dir}/libpng16-16.dll" "${install_files}"
cp "${dll_dir}"/libcrypto-3*.dll "${dll_dir}"/libssl-3*.dll "${install_files}"
cp "${dll_dir}/libqca-qt6.dll" "${install_files}"
cp "${dll_dir}/libsasl2-3.dll" "${install_files}"
cp "${dll_dir}/libldap.dll" "${dll_dir}/liblber.dll" "${install_files}"
cp "${dll_dir}/interception.dll" "${install_files}"
cp "${dll_dir}/liblzo2-2.dll" "${install_files}"
cp "${dll_dir}/libvncclient.dll" "${install_files}"
cp "${dll_dir}/libvncserver.dll" "${install_files}"
cp "${dll_dir_lib}/zlib1.dll" "${install_files}"
cp "${dll_dir_lib}/libwinpthread-1.dll" "${install_files}"
cp "${dll_dir_gcc}/libstdc++-6.dll" "${install_files}"

if [ -f "${dll_dir_gcc}/libssp-0.dll" ]; then
	cp "${dll_dir_gcc}/libssp-0.dll" "${install_files}"
fi

cp "${dll_dir_gcc}/${dll_gcc}" "${install_files}"

mkdir -p "${install_files}/crypto"
qca_ossl_plugin="$(find "${mingw_prefix}" -path '*/crypto/libqca-ossl.dll' -print -quit)"
if [ -n "${qca_ossl_plugin}" ]; then
	cp "${qca_ossl_plugin}" "${install_files}/crypto"
else
	echo "WARNING: libqca-ossl.dll not found under ${mingw_prefix}; continuing without QCA OpenSSL plugin"
fi

cp "${dll_dir}/Qt6Core.dll" \
	"${dll_dir}/Qt6Core5Compat.dll" \
	"${dll_dir}/Qt6Gui.dll" \
	"${dll_dir}/Qt6Widgets.dll" \
	"${dll_dir}/Qt6Network.dll" \
	"${dll_dir}/Qt6Concurrent.dll" \
	"${dll_dir}/Qt6HttpServer.dll" \
	"${dll_dir}/Qt6WebSockets.dll" \
	"${install_files}"

mkdir -p "${install_files}/imageformats"
imageformats_dir="$(qt_plugin_dir imageformats)"
cp "${imageformats_dir}/qjpeg.dll" "${install_files}/imageformats"

mkdir -p "${install_files}/platforms"
platforms_dir="$(qt_plugin_dir platforms)"
cp "${platforms_dir}/qwindows.dll" "${install_files}/platforms"

mkdir -p "${install_files}/styles"
styles_dir="$(qt_plugin_dir styles)"
cp "${styles_dir}"/*.dll "${install_files}/styles"

mkdir -p "${install_files}/tls"
tls_dir="$(qt_plugin_dir tls)"
cp "${tls_dir}/qopensslbackend.dll" "${install_files}/tls"

"${strip_tool}" "${install_files}"/*.dll \
	"${install_files}"/*.exe \
	"${install_files}"/plugins/*.dll \
	"${install_files}"/platforms/*.dll \
	"${install_files}"/styles/*.dll \
	"${install_files}"/crypto/*.dll

cp "${source_dir}/COPYING" "${install_files}"
cp "${source_dir}/COPYING" "${install_files}/LICENSE.TXT"
cp "${source_dir}/README.md" "${install_files}/README.TXT"
todos "${install_files}"/*.TXT

cp -ra "${source_dir}/nsis" "${install_files}"
cp "${binary_dir}/nsis/veyon.nsi" "${install_files}"

find "${install_files}" -ls
