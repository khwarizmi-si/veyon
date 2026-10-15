# Master UI redesign

Branch: `codex/master-ui-redesign`

## Direction

- Keep the live computer grid central. This is an operational classroom console, not a dashboard or a landing page.
- Keep the existing feature execution, selection, licensing, panel state, filters and shortcuts untouched.
- Use labeled mode controls and compact icon actions. Each action retains its Qt tooltip and the built-in toolbar overflow at smaller widths.
- Keep status controls available; switch panel controls to icon-only at widths below 1100 px while retaining their names in tooltips.
- Use neutral surfaces, a green active state and subtle boundaries, with separate light and dark tokens.
- Give Configurator the same restrained light/dark surfaces. Its navigation has smaller icons, a bounded sidebar, elided long plugin names with full-text tooltips, and Indonesian labels on built-in pages and the licence page.

References: [Veyon Master UI](https://docs.veyon.io/en/latest/user/introduction.html),
[NetSupport School Tutor Console](https://help.netsupportschool.com/en-windows/Content/Windows/User-interface/tutor-console.html),
[Apple sidebar guidance](https://developer.apple.com/design/human-interface-guidelines/sidebars).

## QA before merge

1. Build Master in Windows and Linux CI. Verify no compiler warnings from the changed files.
2. At 1024, 1280 and 1920 px, confirm every feature is reachable via the toolbar or Qt overflow; no clipping of the last control.
3. Check light and dark modes, active/checked states, focus, tooltips and text contrast.
4. Exercise monitoring, demo, lock, chat, screenshot, recorder, remote view/control and add-on actions with selected PCs.
5. Check the lower panel toggles, PC search, connection filter, zoom slider and disconnect notice.
6. Restart Master after resizing to ensure geometry and panel visibility still restore correctly.
7. In Configurator, open every built-in and plugin page in standard and advanced modes; confirm long plugin names remain readable via tooltip and forms scroll without horizontal clipping.
8. Activate/check a test licence in Configurator; confirm busy state, status text, save/reset and unsaved-change dialog still work.

Windows CI [run 38663873607](https://github.com/khwarizmi-si/veyon/actions/runs/38663873607)
passed on commit `f88f4dfdf`: licence test, add-on installers and base installer.
The Configurator follow-up passed [run 38665181812](https://github.com/khwarizmi-si/veyon/actions/runs/38665181812)
on commit `7a0e61ef7` with the same gates. A later one-line stylesheet change
restored the native keyboard focus outline; it did not alter Qt APIs or UI wiring.
The local macOS host has no Qt toolchain or running Docker daemon, so live visual
inspection and interactive Windows QA remain before merge.
