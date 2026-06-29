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
	COMMAND bash ${CMAKE_SOURCE_DIR}/.ci/windows/create-base-installer.sh
		${CMAKE_BINARY_DIR}
		${WINDOWS_INSTALL_FILES}
	DEPENDS windows-binaries
)

# Standalone per-add-on installers (Khwarizmi "Plus"). Reuses the same staging
# folder as the base installer so every add-on DLL is built against the exact
# same core (ABI lock). Run this BEFORE create-windows-installer, which deletes
# the staging folder. The Screen Recorder add-on additionally needs an LGPL
# ffmpeg.exe placed at 3rdparty/ffmpeg/ffmpeg.exe (see nsis/README-addons.md).
add_custom_target(create-addon-installers
	COMMAND bash ${CMAKE_SOURCE_DIR}/.ci/windows/create-addon-installers.sh
		${CMAKE_SOURCE_DIR}
		${CMAKE_BINARY_DIR}
		${WINDOWS_INSTALL_FILES}
	DEPENDS windows-binaries
)

add_custom_target(prepare-dev-nsi
	COMMAND sed -i ${WINDOWS_INSTALL_FILES}/veyon.nsi -e "s,/SOLID lzma,zlib,g"
	DEPENDS windows-binaries)

add_custom_target(dev-nsi
	DEPENDS prepare-dev-nsi create-windows-installer
)
