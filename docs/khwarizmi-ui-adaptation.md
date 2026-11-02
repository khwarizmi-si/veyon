# Khwarizmi UI adaptation

## Follow-up: 2026-11-02

Latest localization checkpoint: all 534 remaining legacy entries are now
translated. A source audit also found and added 68 English/technical source
messages absent from the old catalog, including chat, Entra ID, internet
control, discovery and screen recording. lrelease produces 1,266 finished
translations and no unfinished entries. The whole-catalog XML regression
checks nonempty translations and placeholder equality. Both CTest targets
pass; rich-text formatting is preserved. noVNC verifies translated Master
actions and Master/Service settings pages. Windows/device QA remains open.
The observations in the earlier checkpoints below are historical.

Accessibility follow-up: light/dark contrast tests cover input focus against
the Base palette and highlighted text against Highlight. The dark focus
border now uses a brighter green; buttons have explicit keyboard focus
borders. Direct panel icons and ellipsis receive theme-aware colors rather
than retaining black artwork on dark surfaces. Device-action dialog buttons
have autoDefault disabled so the Close button is the Enter default. Verified
in noVNC without dispatching a device action; theme restored to System.

- Complete 64 additional legacy translations for computer selection, power
  confirmations, user sessions, application/website launch and file dialogs.
- Make the System color-scheme option translatable; add Indonesian standard
  Qt dialog button translations (including Close and Cancel).
- Re-enable translation builds in the existing Docker cache with source
  updates disabled, and deploy the freshly built catalog to the build-tree
  runtime location. No production subscription settings were changed.
- The Indonesian preference matches the Configurator language selector.
- Verify the Master suspended-subscription banner in Indonesian in noVNC.
  This closes the previously observed English-banner symptom after reload;
  there is no evidence requiring a change to licensing behavior.
- Extend tests to cover warning and suspended banner messages, eight more
  complete source contexts, and standard dialog button lookup. Both CTest
  targets pass with VEYON_TEST_ID_QM set to the compiled catalog.
- Remaining untranslated legacy messages: 534. Full Windows and device QA
  remain outstanding. Earlier dated observations below are historical.

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
