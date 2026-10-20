# Khwarizmi UI adaptation

## Scope

2026-10-20: replace the Apple-inspired direction with the visual language at
https://ui.khwarizmi.co.id/tokens for the native Qt Master and Configurator.
The Svelte/React package cannot be embedded directly in Qt Widgets. Shared
palette, typography preferences, interaction states and navigation treatment
are adapted without changing device, authentication or licensing behavior.

The desktop uses compact radii and unframed settings sections rather than
copying the website's large card composition. Primary controls use a warm
accent, category icons retain distinct colors, and selection uses a pale tint.
DIN Rounded/Nunito are preferred when installed; existing system fonts remain
the fallback. No web runtime or additional UI dependency is introduced.

Reference alignment pass: the install guide's green primary example replaces
the earlier warm accent. The desktop uses green-700 for white-text contrast,
with green-100 selections. Buttons and fields use the reference's 16px radius.
The generic icon helper no longer recolors icons unless an explicit tint is
provided. Main toolbar roles have distinct green, blue, violet and orange
icons and borders. These are Qt adaptations, not installation of the web
package, and do not yet reproduce its colored base-shadow interaction.

Latest runtime check: both Master and Configurator render this palette in
noVNC; build and license-test pass. Preview was activated with user approval;
the tester subscription is expired and displays a grace-period warning.

## Layout correction

Configurator now excludes hidden pages from active-page size calculations.
The active stack is sized to its viewport or the current page's minimum size,
so unrelated long forms do not stretch the directory tables and push their
action buttons below the visible area. Required scrolling on genuinely long
forms is preserved.

## Verification

Localization pass: compile and install veyon_id.qm in the Docker build-tree
translation path and explicitly configure Indonesian. Previously this preview
used WITH_TRANSLATIONS=OFF without a runtime catalog. Complete Auvidus,
demonstration, general settings, monitoring, remote-access, screen-lock and
screenshot contexts. Source catalog regression checks cover those contexts;
an optional VEYON_TEST_ID_QM check verifies compiled LicenseService lookup.
The catalog currently still has 598 unfinished legacy messages: this pass is
not a claim of complete application localization. Full dialog coverage and
the observed English Master licence banner need further runtime investigation.

The global action dropdown is replaced by a scrollable dialog with grouped,
direct action buttons. Subfeatures are flattened into labelled buttons rather
than cascading menus. Button text wraps; accessible names and tooltips retain
the complete label. Dispatch and disabled state reuse the existing feature
controls. noVNC verified the dialog opens without horizontal overflow; the
license and ToolButton regression tests pass. Device commands were not run.

Mode-control fix: updateModeButtonGroup now synchronizes every current mode,
not only MonitoringMode. Existing ToolButton checked-state labels therefore
switch to the inverse action after selecting a mode via menus as well as the
toolbar. Common mode controls are in a second direct toolbar, not nested in
the global menu. Toolbar lookup now spans both toolbars. Modes other than
monitoring are disabled when there are no connected displayed computers.
toolbutton-test covers alternate-label and exclusive-group reset behavior;
it and license-test pass. Labels represent requested Master mode, not remote
hardware acknowledgement; device-side success still requires end-to-end QA.

Master targeting pass: show the class sidebar on first launch (saved layout
preferences remain respected). A live summary reports displayed, connected,
not-connected and selected computers. A dedicated selected-target action opens
the existing selection context menu and is disabled without selection. The
global menu is explicitly labelled as applying to displayed computers; power
and session commands have a separate submenu. Underlying feature dispatch is
unchanged. noVNC verified the empty summary, disabled selection action, visible
sidebar, and menu grouping. Multi-device selection, command acknowledgements,
failures and disconnect behavior remain unverified in this unactivated preview.

Second structural pass: Master search and connection filters moved from the
status bar into a dedicated workspace header; the class sidebar has its own
title and search at the top. Configurator has a persistent page heading and
category search. Directory table actions now precede table contents. An empty
monitoring result has a visible text status instead of only a watermark.

- Docker Qt 5 builds: Master, Configurator, builtin directory and license test.
- noVNC: Master toolbar icons and the directory page's table action buttons
  render; the directory action row and Apply footer are visible together.
- Production license data was not changed; preview remains unactivated.
- Windows, dark-mode visual coverage, all plugin dialogs, and narrow-window
  coverage still require further QA. This is not a release acceptance report.
