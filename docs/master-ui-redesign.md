# Master UI redesign

Branch: `codex/master-ui-redesign`

## Direction

- Keep the live computer grid central. This is an operational classroom console, not a dashboard or a landing page.
- Keep the existing feature execution, selection, licensing, panel state, filters and shortcuts untouched.
- Use labeled mode controls and compact icon actions. Each action retains its Qt tooltip and the built-in toolbar overflow at smaller widths.
- Qt's [toolbar documentation](https://doc.qt.io/qt-6/qtoolbar.html) confirms that a toolbar owned by `QMainWindow` exposes overflowing `addWidget()` controls through its extension menu. Master creates the toolbar as a main-window child; the hidden actions were opened in Docker at 767 px, with Windows confirmation still pending.
- Keep status controls available; switch panel and mode controls to icon-only below 1250 px, before their expanded minimum width prevents further resizing. Retain their names in tooltips.
- Use neutral surfaces, a green active state and subtle boundaries, with separate light and dark tokens.
- Keep the saved "icons only" preference on startup. Style the location and screenshot panels consistently with Master without changing their models or actions.
- For icon-only feature buttons, show the action name in the existing custom tooltip while respecting the saved "disable tooltips" setting.
- Give Configurator the same restrained light/dark surfaces. Its navigation has smaller icons, a bounded sidebar, elided long plugin names with full-text tooltips, and Indonesian labels on built-in pages and the licence page.
- Tint Configurator navigation icons to the text color in dark mode so monochrome plugin icons remain legible.

References: [Veyon Master UI](https://docs.veyon.io/en/latest/user/introduction.html),
[NetSupport School Tutor Console](https://help.netsupportschool.com/en-windows/Content/Windows/User-interface/tutor-console.html),
[Apple sidebar guidance](https://developer.apple.com/design/human-interface-guidelines/sidebars).

## QA before merge

1. Build Master in Windows and Linux CI. Verify no compiler warnings from the changed files.
2. At 1024, 1280 and 1920 px, confirm every feature is reachable via the toolbar or Qt overflow; no clipping of the last control.
3. Check light and dark modes, active/checked states, focus, tooltips and text contrast.
4. Exercise monitoring, demo, lock, chat, screenshot, recorder, remote view/control and add-on actions with selected PCs.
5. Check the lower panel toggles, PC search, connection filter, zoom slider and disconnect notice.
   Verify keyboard focus and selected rows in the location tree and screenshot list in both themes.
6. Restart Master after resizing to ensure geometry and panel visibility still restore correctly.
7. In Configurator, open every built-in and plugin page in standard and advanced modes; confirm long plugin names remain readable via tooltip and forms scroll without horizontal clipping.
8. Activate/check a test licence in Configurator; confirm busy state, status text, save/reset and unsaved-change dialog still work.

Windows CI [run 38663873607](https://github.com/khwarizmi-si/veyon/actions/runs/38663873607)
passed on commit `f88f4dfdf`: licence test, add-on installers and base installer.
The Configurator follow-up passed [run 38665181812](https://github.com/khwarizmi-si/veyon/actions/runs/38665181812)
on commit `7a0e61ef7` with the same gates. A later one-line stylesheet change
restored the native keyboard focus outline; it did not alter Qt APIs or UI wiring.
The latest [run 38666551425](https://github.com/khwarizmi-si/veyon/actions/runs/38666551425)
passed on commit `41ff333d0`, including the saved icon-only preference and
Master side-panel styling. The workflow still reports upstream Node.js 20 and
MINGW64 deprecation warnings.
Draft [PR #4](https://github.com/khwarizmi-si/veyon/pull/4) targets
`feat/license-client` to keep unrelated licence work out of the UI diff.
Its [Linux CI run 38667339418](https://github.com/khwarizmi-si/veyon/actions/runs/38667339418)
passed on Debian 11 and Fedora 44. Build success does not replace visual and
interaction testing.
The macOS host has no native Qt toolchain. Docker covers Linux rendering, but
interactive Windows QA remains before merge.

## Docker UI check

On the local `veyon-dev` Linux/arm64 image with Qt 5.15, the redesign worktree
compiled `veyon-master`, `veyon-configurator`, `license-test`, the Linux platform
plugin, and representative classroom plugins. `ctest -R '^license-test$'` passed.
Docker Desktop was started for this check, and the worktree's Git submodules
were initialized.

Virtual-desktop screenshots were inspected at 1600x900. Master was resized to
960x700 and its minimum width of 767x600; the status controls remained visible,
the mode buttons became icon-only, and Qt's overflow exposed the remaining
feature actions. Configurator was inspected at 800x650 and 640x600 in light
mode, and its General and Licence pages in dark mode. A too-tall inactive
licence banner and low-contrast dark sidebar icons were found and corrected.

The container has no activated licence or student PC. These checks do not prove
feature behavior, Windows rendering, or the full manual QA list above. The
all-target Docker build also attempts to copy compile commands into the
read-only source mount; targeted application and plugin builds succeeded.
