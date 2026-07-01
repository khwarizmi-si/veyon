; addon-common.nsh - shared logic for standalone Khwarizmi add-on installers
;
; Each add-on installer (.nsi) defines the following before including this file:
;   ADDON_NAME       e.g. "Chat"            (display name + registry/uninstaller id)
;   ADDON_DLL        e.g. "chat.dll"        (plugin DLL to install into plugins\)
;   ADDON_VERSION    e.g. "1.0"
;   KH_ARCH          "win64" / "win32"      (filled from CMake)
;   KH_SETREGVIEW    "SetRegView 64" / ""   (filled from CMake)
; Optional, for add-ons that ship an extra executable next to the core (e.g. ffmpeg):
;   ADDON_EXTRA_FILE host path to the file to bundle, copied into the install root
;
; The add-on is just one plugin DLL dropped into the existing Khwarizmi
; installation. The base application MUST already be installed - this installer
; locates it through the registry key the base installer writes.

!define KH_APP        "Khwarizmi"
!define KH_MAIN_EXE   "veyon-master.exe"
!define KH_CLI        "veyon-wcli.exe"
!define KH_APP_PATH   "Software\Microsoft\Windows\CurrentVersion\App Paths\${KH_MAIN_EXE}"
!define ADDON_UNINST  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${KH_APP}-${ADDON_NAME}"

Name "${KH_APP} ${ADDON_NAME} Add-on"
Caption "${KH_APP} ${ADDON_NAME} Add-on ${ADDON_VERSION}"
BrandingText "${KH_APP} ${ADDON_NAME} Add-on"
OutFile "${KH_APP}-${ADDON_NAME}-Addon-${ADDON_VERSION}-${KH_ARCH}-setup.exe"
SetCompressor /SOLID lzma
RequestExecutionLevel admin
ShowInstDetails show
ShowUninstDetails show

Var KH_DIR ; resolved Khwarizmi installation directory

!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "MUI2.nsh"

!define MUI_ICON "nsis\installer.ico"
!define MUI_UNICON "nsis\uninstaller.ico"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

######################################################################
# locate the base installation and refuse to run without it / without admin

Function .onInit
	${KH_SETREGVIEW}
	UserInfo::GetAccountType
	pop $0
	${If} $0 != "admin"
		MessageBox MB_ICONSTOP "Please run this installer with administrative rights."
		SetErrorLevel 740 ; ERROR_ELEVATION_REQUIRED
		Quit
	${EndIf}

	# the base installer stores the full path to veyon-master.exe here
	ReadRegStr $0 HKLM "${KH_APP_PATH}" ""
	${If} $0 == ""
		MessageBox MB_ICONSTOP "Khwarizmi is not installed.$\n$\nPlease install the Khwarizmi base application before installing this add-on."
		Quit
	${EndIf}
	${GetParent} "$0" $KH_DIR ; strip \veyon-master.exe -> installation directory
	${IfNot} ${FileExists} "$KH_DIR\${KH_MAIN_EXE}"
		MessageBox MB_ICONSTOP "Could not locate the Khwarizmi installation directory."
		Quit
	${EndIf}

	MessageBox MB_OKCANCEL "This will install the ${ADDON_NAME} add-on into:$\n$KH_DIR$\n$\nPlease close Khwarizmi Master before continuing." IDOK +2
	Quit
FunctionEnd

######################################################################

Section "Install ${ADDON_NAME} add-on"
	SetShellVarContext all

	# stop the service so the plugin DLL is not locked while we replace it
	ExecWait '"$KH_DIR\${KH_CLI}" service stop'
	Sleep 1500

	SetOverwrite on
	SetOutPath "$KH_DIR\plugins"
	!ifdef ADDON_DLL_PATH
		File "${ADDON_DLL_PATH}"
	!else
		File "plugins\${ADDON_DLL}"
	!endif

	!ifdef ADDON_EXTRA_FILE
		# extra helper next to the executables (Windows resolves it from the app dir)
		SetOutPath "$KH_DIR"
		File "${ADDON_EXTRA_FILE}"
	!endif

	# per-add-on uninstaller + Add/Remove Programs entry
	SetOutPath "$KH_DIR"
	WriteUninstaller "$KH_DIR\uninstall-${ADDON_NAME}.exe"
	${KH_SETREGVIEW}
	WriteRegStr HKLM "${ADDON_UNINST}" "DisplayName" "${KH_APP} ${ADDON_NAME} Add-on"
	WriteRegStr HKLM "${ADDON_UNINST}" "DisplayVersion" "${ADDON_VERSION}"
	WriteRegStr HKLM "${ADDON_UNINST}" "Publisher" "${KH_APP}"
	WriteRegStr HKLM "${ADDON_UNINST}" "UninstallString" "$KH_DIR\uninstall-${ADDON_NAME}.exe"

	# bring the service back up so the new feature is available immediately
	ExecWait '"$KH_DIR\${KH_CLI}" service start'
SectionEnd

######################################################################

Section "Uninstall"
	SetShellVarContext all
	${KH_SETREGVIEW}

	ExecWait '"$INSTDIR\${KH_CLI}" service stop'
	Sleep 1500

	Delete "$INSTDIR\plugins\${ADDON_DLL}"
	!ifdef ADDON_EXTRA_FILE_NAME
		Delete "$INSTDIR\${ADDON_EXTRA_FILE_NAME}"
	!endif
	Delete "$INSTDIR\uninstall-${ADDON_NAME}.exe"
	DeleteRegKey HKLM "${ADDON_UNINST}"

	ExecWait '"$INSTDIR\${KH_CLI}" service start'
SectionEnd
