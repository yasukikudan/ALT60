# Hand-wired keyboard firmware bring-up — agent instructions

This repo is a small framework for bringing a hand-wired mechanical
keyboard (Pro Micro / ATmega32U4, or any Arduino-Leonardo-compatible board)
from "pile of switches and wires" to a working USB keyboard, with an AI
agent doing the interactive pin discovery together with the person who
built it. If you're an agent picking this up for a *new* board, this file
is the whole playbook.

## The core problem this solves

Someone hand-wired a keyboard matrix. They don't know which physical key
maps to which microcontroller pin pair, because there was no PCB silkscreen
to tell them — they just soldered wires. You can't write a keymap without
that mapping, and they can't discover it alone with a multimeter in any
reasonable time. The fix: flash a firmware that sweeps every pin against
every other pin, have the person press one key at a time, read back which
pin pair lit up, and build the map together, live.

## Repo layout

- `matrix_scanner/matrix_scanner.ino` — the discovery tool. Drives every
  candidate pin low in turn, reads every other candidate pin, and prints
  `KEY DETECTED out=Dx in=Dy` over Serial the moment a press closes a
  circuit, plus a once-a-second `SNAPSHOT: ...` of anything currently
  closed (catches stuck/shorted pins even with nothing pressed). Not the
  daily-driver firmware — only used during bring-up and re-diagnosis.
- `keyboard_fw/` — the real firmware. Three files:
  - `board_config.h` — **the only file a new board needs to edit.** Pin
    arrays, the keymap, the Fn layer, JIS compensation positions, LED pin.
    Fully commented; read it before asking the user anything the file
    already answers.
  - `matrix_scan.h` — generic input layer (scan, debounce, chatter stats).
    Don't edit this to support a new board.
  - `keycode_output.h` — generic output layer (keycode dispatch, Fn layer
    logic, JIS fix-ups). Don't edit this to support a new board either —
    if you find yourself wanting to, the thing you're adding probably
    belongs in `board_config.h` as a new position constant plus a small
    generic handler, not a board-specific branch.
  - `keyboard_fw.ino` — thin glue, rarely needs touching.
- `keymap_notes.md`, `keymap_by_pin.md` — human-readable record of a
  specific board's discovered pin map. Regenerate the equivalent for each
  new board; it's the paper trail for "why is pin D7 doing that."

## The bring-up workflow, step by step

1. **Toolchain, once per machine.** `arduino-cli` (core `arduino:avr`) via
   the system package manager, `HID-Project` via `arduino-cli lib install
   "HID-Project"`. Board enumerates as an Arduino Leonardo
   (`VID_2341&PID_8036`) even on genuinely different hand-wired boards, so
   `--fqbn arduino:avr:leonardo` reliably works without needing a
   board-specific package (e.g. SparkFun's).

2. **Find the device.** It shows up as a USB serial port. On Windows,
   `Get-WmiObject Win32_PnPEntity | Where-Object { $_.PNPDeviceID -match
   "VID_2341" }` finds the COM port. It *will* renumber on every reflash —
   always re-check before the next upload/monitor rather than assuming the
   port from ten minutes ago.

3. **Flash `matrix_scanner.ino`**, confirm it's alive (open the port, look
   for the boot banner or idle `SNAPSHOT: (none)` lines).

4. **Discover pins one key at a time, live, with the person.** This is the
   part that actually takes the session's time and where the failure modes
   live — see "Interactive discovery, what actually works" below.

5. **Fill in `board_config.h`** from what you found: `rowPins[]`/
   `colPins[]` (physical pins, in whatever order you scanned them —
   everything else refers to *index into these arrays*, not raw pin
   numbers), then `keymap[][]` using those same indices.

6. **Flash `keyboard_fw`, test in a real text field**, not just via the
   serial monitor — a keycode reaching the firmware correctly is not the
   same as it reaching the OS as the right character (see the JIS section
   below for why that distinction matters).

7. **Iterate.** Expect several rounds of "this key doesn't work" as the
   person notices things while actually typing that isolated per-key
   testing didn't surface (chording issues, JIS/US mismatches, wiring
   faults that only show up once everything's connected).

## Interactive discovery, what actually works

This is the part worth reading carefully — a lot of approaches that sound
reasonable on paper wasted real time before something better emerged.

**Round-trip latency kills naive "press now, I'm listening" loops.** A
short PowerShell/pyserial listen window that starts *after* you tell the
person to press, ends *before* they've actually pressed, and reports
nothing, is nearly free to fall into and wastes a turn every time it
happens. What works: after telling them to press, open the listen window
immediately and give it a generous window (15–20s) rather than trying to
time it precisely. If it comes back empty, just ask them to press again
without changing anything else.

**A long-lived backgrounded serial-logger process is more trouble than
it's worth in this environment.** It sounds appealing (start once, keep
tailing a log file, no more round-trip races) but in practice spawns
multi-process chains that are hard to clean up, silently double up, or
hold the port open and block your *next* upload. Prefer short-lived
open→read-for-N-seconds→close calls per key. If you want a live tail the
person can watch themselves, hand them the raw PowerShell snippet and let
them run it in their own terminal instead of trying to background it
yourself.

**No keycaps makes verbal key identification genuinely unreliable.**
Without keycaps, "which switch is Q" is a real question, not a formality —
expect the person to occasionally press the wrong physical switch, or
describe a batch of presses in an order that doesn't match what actually
got detected. Symptoms: a "known" key suddenly reports a different pin, or
a batch test returns more or fewer detections than keys pressed. Don't
assume the wiring changed — first suspect a mismatch between what they
think they pressed and what they actually pressed, and re-test the
specific key in isolation. Once real keycaps are on, re-verify anything
that was contested before — it resolves most of these disagreements
immediately, and don't be surprised if a couple of contested identities
flip once verbal ambiguity is gone.

**Ghosting/ordering artifacts vs. real chatter vs. real wiring faults —
tell them apart before reacting.** A rapid-fire cascade of several *different*
pin pairs in one detection window can be either (a) the person genuinely
sweeping through a physical row/column of keys quickly, which is real,
reproducible data — or (b) noise. The tell: real sweeps reproduce in the
same relative order across repeated attempts; noise doesn't, and tends to
implicate pins whose own idle `SNAPSHOT` shows them closed with nothing
pressed at all. Check idle `SNAPSHOT` state before concluding a pin is
flaky. A pin pair that's closed at idle with zero user input is a genuine
wiring defect (short/stray contact), not a firmware problem — don't try to
debounce or filter your way around it; tell the person to physically
inspect that joint.

**Full-pin-scan diagnostics can catch cross-role shorts a "real" matrix
scan never would.** Because `matrix_scanner.ino` tries every pin as both
output and input, it'll surface a short between two pins that, in the real
firmware, are *both always driven as rows and never read as columns* —
production code would never notice this directly, but it still causes real
symptoms (one row's keys interfering with another's) because driving one
of the shorted pins pulls the other along with it. If two keys that share
nothing in the real keymap start misbehaving together, suspect exactly
this, and check whether their row pins are physically adjacent on the
microcontroller's header — adjacent-pin solder bridges are the single most
common cause.

**A clean, consistent swap across many keys means the fix is one line, not
many.** If several keys all show the *other* column/row's pin consistently
(not intermittently), the wires got crossed at the microcontroller end, not
at each individual key. Confirm by testing two keys from the same
suspected column, both isolated. If both come back wrong the same way, fix
it once in `rowPins[]`/`colPins[]` — do **not** patch individual `keymap[][]`
entries for each affected key; that fixes the symptom key-by-key and
silently breaks again if you later touch the array order.

**A page with a `keydown` listener is a better oracle than asking the user
what character appeared.** For verifying what actually reaches the OS
(keycodes, not raw pin pairs), open a tiny local HTML page (`key_tester.html`
in this repo) with the Claude_Browser tool and read
`event.key`/`event.code` from the console after each press, instead of
relying on the person to describe/type out what showed up. This was the
difference between guessing and *knowing* for both an F-key layer
regression and a genuine multi-key race condition later in this project's
history — see `keymap_notes.md` / the memory file for the specifics.

## Debounce and scan rate

Defaults in `board_config.h` (`DEBOUNCE_MS`, `SCAN_INTERVAL_MS`) were tuned
empirically on one board with real chattering hardware — treat them as a
starting point, not gospel. If the person reports chattering (doubled or
dropped characters), use the built-in chatter-rate stats
(`MatrixScan::printChatterStats()`, gated behind `if (Serial)` so it's safe
to leave in) before touching debounce blindly: report a *rate*
(chatters/press, as %), not a raw count — raw counts are dominated by how
often a key is actually typed (common letters rack up big numbers that
look alarming but aren't). A raw chatter count without press-count context
will mislead you into chasing the wrong key. If the rate is high and
reproducible on one specific key even after raising debounce, that's a
hardware connection issue (loose/cold solder joint) firmware can't fully
paper over — say so plainly instead of continuing to tune debounce values.

## `Serial.print` is dangerous in production firmware — never leave it in

On this USB CDC + HID composite setup, `Serial.print()` **blocks
indefinitely once its internal TX buffer fills if nothing has opened the
port to read it** — which freezes the *entire* keyboard (all HID output
stops too, since it's the same `loop()`). This actually happened once
during this project's development: added debug prints, forgot to remove
them, the keyboard went completely unresponsive the moment nobody was
watching a terminal. The safe patterns:
- Guard any print with `if (Serial) { ... }` — this is only true once a
  host has actually asserted DTR by opening the port, so it can never
  block when nothing's listening. `printChatterStats()` uses this pattern
  and is safe to leave in permanently.
- If you add *temporary* unconditional debug prints for a specific
  investigation, remove them before calling the firmware change done —
  don't rely on remembering to guard them "for now."

## JIS/US keyboard-layout compensation (optional feature, board_config.h-driven)

If the host OS's keyboard layout setting doesn't match the physical board
(most commonly: Windows set to Japanese/JIS, board wired/labelled US
ANSI), symbol keys print the wrong character even though the firmware is
sending the textbook-correct US HID code — the *OS* reinterprets it under
the wrong layout. `Fn+Menu` toggles a compensation mode
(`keycode_output.h`'s JIS handlers) that substitutes different raw HID
codes so the *visible result* matches the US legend on the keycap. This
was worked out empirically for JIS by testing every affected key live and
is fully documented inline in `keycode_output.h` — read those comments
before assuming you know what a given substitution does, several of them
are deliberately non-obvious (e.g. the shifted digit row uses a "send the
next digit's key" chain pattern that looks wrong until you see the
derivation). If a *different* target layout is ever needed (not JIS), the
right move is a parallel set of handler functions following the same
pattern, not modifying the JIS ones in place.

## Designing a clean wiring plan (not just discovering an existing one)

If someone is about to physically wire (or rewire) a board from scratch,
push back gently on wiring it opportunistically key-by-key the way this
project's own board ended up (rows/columns with no consistent pattern —
see `keymap_notes.md`'s "正しい配線プラン" section for a worked example
of the mess that causes and the fix). A clean plan just needs two rules:
one axis's pins each map to one physical row of keys, the other axis's
pins map to left-to-right position within a row. Work out the max keys in
any single row up front — if it exceeds the position-axis pin count (a
real risk on boards with 13-14 keys in a row but only 13ish spare pins),
document the 1-2 unavoidable exceptions explicitly (borrow an unused slot
from a shorter row) rather than letting the whole scheme go ad-hoc to
route around it. A plan like this only pays off if the physical wiring
actually gets built to match it — don't silently rewrite `board_config.h`
to a new clean scheme unless the physical rewiring happened first, or the
firmware and the real board will disagree.

## What NOT to do

- Don't guess at a pin mapping instead of measuring it, even when a
  pattern from earlier rows makes a guess tempting — hand-wiring order
  doesn't reliably follow physical key order, and this project's actual
  history is full of confident-looking patterns that were coincidences.
- Don't treat "the diagnostic firmware confirms it" and "the production
  firmware behaves correctly" as the same claim — always do a final pass
  typing into a real text field.
- Don't use destructive git/serial-port operations to route around a
  stuck state (e.g. force-killing every powershell.exe) without checking
  what else might be relying on it first.
