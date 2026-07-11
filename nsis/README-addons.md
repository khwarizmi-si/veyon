# Khwarizmi — Basic vs Plus packaging

Each add-on is just **one plugin DLL** dropped into `…\Khwarizmi\plugins\`.
Khwarizmi loads every `*.dll` in that folder on startup, so add-ons are
installed and removed independently of the base application.

## Packages

| Package | Contents |
|---------|----------|
| **Khwarizmi Basic** | `veyon-*-setup.exe` — core + free built-in plugins. The base installer explicitly removes the add-on DLLs (`chat`, `internetaccesscontrol`, `screenrecorder`, `networkdiscovery`, `auvidus`, `entraid`). |
| **Add-on: Chat** | `Khwarizmi-Chat-Addon-*-setup.exe` — drops `chat.dll`. |
| **Add-on: Internet Access Control** | drops `internetaccesscontrol.dll`. |
| **Add-on: Screen Recorder** | drops `screenrecorder.dll` **and** `ffmpeg.exe`. |
| **Add-on: Network Discovery** | drops `networkdiscovery.dll`. After installing, select **Network Discovery** as the directory backend in the Configurator (Network settings) to activate it. |
| **Add-on: Auvidus** | drops `auvidus.dll` (audio/webcam/USB control). Windows-only device control. |
| **Add-on: Entra ID** | drops `entraid.dll`. Configure tenant, app credentials and device mapping in the Configurator, then select **Microsoft Entra ID** as the directory and/or user groups backend. |

Sell Basic on its own; sell/deploy add-ons on top as "Plus". An add-on
installer refuses to run unless the Basic application is already installed.

## ⚠️ ABI lock (must read)

An add-on DLL links against `veyon-core`, so it **only works with the exact
Khwarizmi version it was built against**. Always:

1. Build the base app and all add-ons from the **same source tree / same build**.
2. Version-lock each add-on release to a Khwarizmi version.
3. Re-build **all** add-ons whenever the core is updated.

This is why the source for every add-on lives together on one branch — see
`plugins/chat`, `plugins/internetaccesscontrol`, `plugins/screenrecorder`,
`plugins/networkdiscovery`, `plugins/auvidus`, and `plugins/entraid`.

## Building the installers (Windows / MSYS2)

From the build directory, with NSIS (`makensis`) available:

```sh
# 1. Base (Basic) installer
make windows-binaries
make create-addon-installers   # build add-on installers from the SAME staging
make create-windows-installer  # build Basic installer (also deletes staging)
```

Run `create-addon-installers` **before** `create-windows-installer`, because the
latter deletes the staging folder both share.

### Screen Recorder needs ffmpeg.exe

The Screen Recorder pipes frames to `ffmpeg`. Download an **LGPL** Windows build
and place the binary at:

```
3rdparty/ffmpeg/ffmpeg.exe
```

The installer puts it next to the executables, where Windows resolves it for the
`ffmpeg` process the recorder launches. If the file is missing, the Screen
Recorder installer is skipped (Chat and Internet Access Control still build).
Per FFmpeg's LGPL terms, ship the corresponding source offer alongside.

## How an add-on installer works

1. Reads the Khwarizmi install path from the registry
   (`HKLM\…\App Paths\veyon-master.exe`) and aborts if the base app is missing.
2. Stops the Khwarizmi service so the plugin DLL is not locked.
3. Copies the DLL into `plugins\` (and `ffmpeg.exe` for Screen Recorder).
4. Writes its own Add/Remove Programs entry + uninstaller.
5. Restarts the service.

Close **Khwarizmi Master** before installing — the master process also loads the
plugin DLLs.

## Windows build checklist

The add-on plugins are picked up automatically by the normal Windows build (the
`plugins/*` glob), so the only extra steps are the installer targets:

1. Drop an LGPL `ffmpeg.exe` at `3rdparty/ffmpeg/ffmpeg.exe` (Screen Recorder).
2. Build the staged binaries: `make windows-binaries`.
3. Build the add-on installers from that staging: `make create-addon-installers`.
4. Build the Basic installer (this deletes the staging): `make create-windows-installer`.

You get one Basic `.exe` plus one `.exe` per add-on (Chat, Internet Access
Control, Screen Recorder, Network Discovery, Auvidus, Entra ID).

### Qt note

The dev build (Docker) is Qt5; the Windows build is **Qt6/MinGW**. All add-on
code was audited and the Qt-heavy parts compile clean under Qt6. The only code
that could not be checked off-Windows is the Auvidus device control (see below).

## On-device verification (run once on real Windows hardware)

These paths can only be confirmed on Windows — verify each after deploying:

| Add-on | What to confirm |
|--------|-----------------|
| Chat | two-way teacher↔student messaging (already proven on Linux too) |
| Internet Access Control | the **netsh** firewall path actually blocks internet while the Khwarizmi link stays up; unblock restores it |
| Screen Recorder | `ffmpeg.exe` is found next to the executables and produces a valid MP4 |
| Network Discovery | the subnet scan lists the lab PCs (select it as the directory backend) |
| **Auvidus** | the Windows-only device control — **untested off-Windows**: USB block (`USBSTOR\Start` registry), webcam consent registry, and the **Core Audio COM** mute path. Confirm it compiles under MinGW and each toggle works. |
| Entra ID | test with a real Microsoft tenant: app credentials, admin consent, device sync, and security group access control |
