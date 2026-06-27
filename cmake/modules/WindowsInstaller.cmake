set(WINDOWS_INSTALL_FILES "veyon-${VEYON_WINDOWS_ARCH}-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD}")

set(DLLDIR "${MINGW_PREFIX}/bin")
set(DLLDIR_LIB "${MINGW_PREFIX}/lib")
string(REGEX MATCH "^[^.]+" GCC_VERSION_MAJOR ${CMAKE_CXX_COMPILER_VERSION})
set(DLLDIR_GCC "/usr/lib/gcc/${MINGW_TARGET}/${GCC_VERSION_MAJOR}-posix")
if(VEYON_BUILD_WIN64)
	set(DLL_GCC "libgcc_s_seh-1.dll")
	set(DLL_DDENGINE "ddengine64.dll")
else()
	set(DLL_GCC "libgcc_s_dw2-1.dll")
	set(DLL_DDENGINE "ddengine.dll")
endif()

add_custom_target(windows-binaries
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIGURATION>
	COMMAND bash ${CMAKE_SOURCE_DIR}/.ci/windows/stage-installer.sh
		${CMAKE_SOURCE_DIR}
		${CMAKE_BINARY_DIR}
		${WINDOWS_INSTALL_FILES}
		${DLLDIR}
		${DLLDIR_LIB}
		${DLLDIR_GCC}
		${DLL_GCC}
		${DLL_DDENGINE}
		${MINGW_PREFIX}
		${MINGW_TOOL_PREFIX}strip
)

add_custom_target(create-windows-installer
	COMMAND makensis ${WINDOWS_INSTALL_FILES}/veyon.nsi
	COMMAND mv ${WINDOWS_INSTALL_FILES}/veyon-*setup.exe .
	COMMAND rm -rf ${WINDOWS_INSTALL_FILES}
	DEPENDS windows-binaries
)

# Standalone per-add-on installers (Sahid "Plus"). Reuses the same staging
# folder as the base installer so every add-on DLL is built against the exact
# same core (ABI lock). Run this BEFORE create-windows-installer, which deletes
# the staging folder. The Screen Recorder add-on additionally needs an LGPL
# ffmpeg.exe placed at 3rdparty/ffmpeg/ffmpeg.exe (see nsis/README-addons.md).
add_custom_target(create-addon-installers
	COMMAND cp ${CMAKE_BINARY_DIR}/nsis/addon-chat.nsi ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_BINARY_DIR}/nsis/addon-internetaccesscontrol.nsi ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_BINARY_DIR}/nsis/addon-networkdiscovery.nsi ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_BINARY_DIR}/nsis/addon-auvidus.nsi ${WINDOWS_INSTALL_FILES}
	COMMAND cp ${CMAKE_BINARY_DIR}/nsis/addon-screenrecorder.nsi ${WINDOWS_INSTALL_FILES}
	COMMAND makensis ${WINDOWS_INSTALL_FILES}/addon-chat.nsi
	COMMAND makensis ${WINDOWS_INSTALL_FILES}/addon-internetaccesscontrol.nsi
	COMMAND makensis ${WINDOWS_INSTALL_FILES}/addon-networkdiscovery.nsi
	COMMAND makensis ${WINDOWS_INSTALL_FILES}/addon-auvidus.nsi
	COMMAND sh -c "cp ${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/ffmpeg.exe ${WINDOWS_INSTALL_FILES}/ && makensis ${WINDOWS_INSTALL_FILES}/addon-screenrecorder.nsi || echo 'SKIP ScreenRecorder installer: place an LGPL ffmpeg.exe at 3rdparty/ffmpeg/ffmpeg.exe'"
	COMMAND mv ${WINDOWS_INSTALL_FILES}/Sahid-*-Addon-*setup.exe .
	DEPENDS windows-binaries
)

add_custom_target(prepare-dev-nsi
	COMMAND sed -i ${WINDOWS_INSTALL_FILES}/veyon.nsi -e "s,/SOLID lzma,zlib,g"
	DEPENDS windows-binaries)

add_custom_target(dev-nsi
	DEPENDS prepare-dev-nsi create-windows-installer
)
