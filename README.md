# ALT60

A 61-key, HHKB-inspired 60% hand-wired mechanical keyboard, designed to be
3D-printed and built by hand — no PCB. This repo is both the design spec
for ALT60 itself and a small reusable framework for bringing up *any*
hand-wired Pro Micro keyboard from scratch.

ALT60 grew out of an earlier hand-wired prototype ("ALTKB"), built by
discovering its pin mapping interactively, key by key, with no upfront
plan. That process worked but left the wiring hard to read after the
fact. ALT60 is the do-it-properly version: a clean, documented wiring
scheme, a matching firmware config, and a 3D-printable case designed
around it from the start.

## What's here

| | |
|---|---|
| [`docs/wiring_plan.html`](docs/wiring_plan.html) | The full 13×5 matrix wiring table — which pin pair each of the 61 keys sits on, and why. |
| [`docs/wiring_layout.html`](docs/wiring_layout.html) | The same wiring, overlaid on the physical key layout — meant to be open while soldering. |
| [`docs/keymap.html`](docs/keymap.html) | The logical keymap: base layer, Fn layer, JIS/US symbol compensation. |
| [`docs/case_design.html`](docs/case_design.html) | 3D-printable case spec — plate, walls, screw bosses, MCU pocket, typing-angle wedge, all dimensioned. |
| [`firmware/`](firmware/) | The firmware. `keyboard_fw/board_config.h` is ALT60's config; `matrix_scan.h`/`keycode_output.h` are generic. |
| [`CLAUDE.md`](CLAUDE.md) | Agent instructions for bringing up a hand-wired board with AI-assisted pin discovery — written to be reusable for a *different* board too. |

## Keyboard summary

- **61 keys**, 15U × 5-row HHKB-style layout (Row1=14, Row2=14, Row3=13, Row4=12, Row5=8 keys).
- **No stabilizer on the spacebar** — two independent switches under the 6.25U keycap, only one wired into the matrix; the other is a mechanical-support-only dummy switch.
- **Full Fn layer**: F-keys, arrows, navigation cluster, media keys, and independent grave (`` ` ``, Fn+Z) / tilde (`~`, Fn+X) bindings — researched against HHKB's own layout rather than guessed.
- **Runtime JIS/US symbol compensation** (`Fn+Menu`, persisted to EEPROM) for boards used under a Japanese OS keyboard layout despite US-legend keycaps.
- **NKRO** over USB via [HID-Project](https://github.com/NicoHood/HID)'s `NKROKeyboard`.

## Wiring, in one sentence

Column pins (`D5`–`D9`) each drive one physical keyboard row; row pins
(`D0`,`D1`,`D2`,`D3`,`D4`,`D10`,`D14`,`D15`,`D16`,`D18`,`D19`,`D20`,`D21`)
select left-to-right position within a row. Two exceptions — Backspace
and `\`, the 14th key in rows that only have 13 row-pin slots — borrow
Row5's otherwise-unused row-pin slots. Full detail and rationale:
[`docs/wiring_plan.html`](docs/wiring_plan.html).

## Case

3D-printed, two parts: a top plate (switch holes only) and a bottom
shell (integrated side walls, a recessed pocket for a small Pro Micro +
perfboard sub-assembly, screw bosses molded into the walls — no loose
standoff hardware). The typing angle (~7.45°, front 18.0mm / back
33.1mm) follows HHKB's own published dimensions. Full spec, including
every wall/boss/hole dimension: [`docs/case_design.html`](docs/case_design.html).

## Building the firmware

```bash
arduino-cli core install arduino:avr
arduino-cli lib install "HID-Project"
arduino-cli compile --fqbn arduino:avr:leonardo firmware/keyboard_fw
arduino-cli upload -p <COM_PORT> --fqbn arduino:avr:leonardo firmware/keyboard_fw
```

`firmware/keyboard_fw/board_config.h` is ALT60's target configuration,
derived directly from `docs/wiring_plan.html` — it has **not yet been
verified against a physically wired board**. If you're building an
ALT60, wire it per `docs/wiring_layout.html`, then flash
`firmware/matrix_scanner/matrix_scanner.ino` first and confirm each key
actually lands on the (row, col) pair `board_config.h` expects before
trusting it — see `CLAUDE.md` for the full bring-up workflow.

## Building a *different* hand-wired board with this framework

`firmware/keyboard_fw/matrix_scan.h` and `keycode_output.h` are fully
generic — they read everything board-specific from `board_config.h`.
`firmware/matrix_scanner/matrix_scanner.ino` is the interactive
pin-discovery tool. `CLAUDE.md` documents the whole process (including
the failure modes that actually cost time the first time around) so an
AI agent can run the same bring-up workflow for someone else's board.

## License

BSD 3-Clause — see [`LICENSE`](LICENSE). Fill in the copyright holder name before publishing.
