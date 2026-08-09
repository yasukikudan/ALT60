#pragma once
#include "HID-Project.h"

// ============================================================================
// BOARD CONFIG for ALT60.
//
// This is the *target* configuration derived from ALT60's wiring plan
// (see ../docs/wiring_plan.html and ../docs/wiring_layout.html) -- it has
// NOT been verified against a physically wired board yet. Follow the
// bring-up workflow in ../CLAUDE.md: flash matrix_scanner/, confirm each
// (row, col) pair actually lands where this file expects, then trust it.
// If anything disagrees, the physical wiring or this file is wrong --
// don't assume this file is correct just because it's here.
//
// Wiring rule this config follows (see docs/wiring_plan.html for the full
// rationale): colPins[] index = physical keyboard row (Row1..Row5, one
// pin per row). rowPins[] index = left-to-right position within a row --
// except Row5, whose 8 keys are NOT simply positions 0-7: the spacebar
// eats 6.25U in one slot, so Row5's later keys sit far to the right
// physically, and using 0-7 would force those row-pin wires into long
// rightward detours. Row5 instead uses whichever position index sits
// physically closest to each key (0,1,3,6,9,10,11,12), leaving 7 and 8
// genuinely free for the two documented exceptions (Backspace, \) --
// Row1/Row2 have 14 keys each but there are only 13 row pins, so those
// two borrow Row5's column at those otherwise-unused slots. Full
// rationale and the physical-position math: docs/wiring_plan.html.
// ============================================================================

namespace BoardConfig {

#define FN_LAYER 0xFF // sentinel: this position is Fn, never sent as HID

// --- Matrix wiring ---------------------------------------------------------
#define NUM_ROWS 13
#define NUM_COLS 5

// Row pins = position-within-row 0..12, left to right.
const uint8_t rowPins[NUM_ROWS] = {0, 1, 2, 3, 4, 10, 14, 15, 16, 18, 19, 20, 21};
// Col pins = physical row, Row1..Row5.
const uint8_t colPins[NUM_COLS] = {5, 6, 7, 8, 9};

// Shift lives in col3 (Row4), Fn lives in col4 (Row5) -- dispatch those
// columns first each scan pass to reduce same-row modifier-timing races.
const uint8_t PRIORITY_COL_A = 3;
const uint8_t PRIORITY_COL_B = 4;

// --- Debounce / scan rate ---------------------------------------------------
const unsigned long DEBOUNCE_MS = 5;
const unsigned long SCAN_INTERVAL_MS = 1; // ~1000Hz
const unsigned long CHATTER_WINDOW_MS = 300; // stats only, see matrix_scan.h

// --- Base keymap -------------------------------------------------------------
// keymap[row][col], 0 = no key at that position, FN_LAYER = this position
// is the Fn key itself (sends nothing on its own).
// row = position-within-row (0..12), col = physical row (0=Row1..4=Row5).
// col4 (Row5) positions are NOT a simple 0-7 left-to-right run like the
// other columns. The spacebar occupies 6.25U in one slot, so Row5's later
// keys (Ctrl/Alt/Menu/Fn) sit far to the right physically -- using
// positions 0-7 would force every one of those row-pin wires to detour
// increasingly far right to reach them. Instead each Row5 key uses the
// row-pin index whose typical physical location (in the other columns)
// is closest to that key's real position: 0,1,3,6,9,10,11,12 (skipping
// 2,4,5 -- genuinely unused -- and 7,8, borrowed below by Backspace/\).
const uint8_t keymap[NUM_ROWS][NUM_COLS] = {
  /* pos0  */ {KEY_ESC,   KEY_TAB,        KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_CAPS_LOCK}, // col4: base=Caps(US)/Zenkaku-Hankaku(JIS); Fn+this=Win/Meta
  /* pos1  */ {KEY_1,     KEY_Q,          KEY_A,         KEY_Z,          KEY_LEFT_ALT},
  /* pos2  */ {KEY_2,     KEY_W,          KEY_S,         KEY_X,          0},
  /* pos3  */ {KEY_3,     KEY_E,          KEY_D,         KEY_C,          KEY_LEFT_CTRL},
  /* pos4  */ {KEY_4,     KEY_R,          KEY_F,         KEY_V,          0},
  /* pos5  */ {KEY_5,     KEY_T,          KEY_G,         KEY_B,          0},
  /* pos6  */ {KEY_6,     KEY_Y,          KEY_H,         KEY_N,          KEY_SPACE},
  /* pos7  */ {KEY_7,     KEY_U,          KEY_J,         KEY_M,          KEY_BACKSPACE},   // col4: exception, borrows Row5's unused slot
  /* pos8  */ {KEY_8,     KEY_I,          KEY_K,         KEY_COMMA,      KEY_BACKSLASH},   // col4: exception, borrows Row5's unused slot
  /* pos9  */ {KEY_9,     KEY_O,          KEY_L,         KEY_PERIOD,     KEY_RIGHT_CTRL},
  /* pos10 */ {KEY_0,     KEY_P,          KEY_SEMICOLON, KEY_SLASH,      KEY_RIGHT_ALT},
  /* pos11 */ {KEY_MINUS, KEY_LEFT_BRACE, KEY_QUOTE,     KEY_RIGHT_SHIFT, KEY_MENU},       // Fn+Menu toggles JIS mode
  /* pos12 */ {KEY_EQUAL, KEY_RIGHT_BRACE, KEY_RETURN,   0,              FN_LAYER},        // Fn: sends nothing on its own
};

// --- JIS compensation (optional feature) -----------------------------------
// jisOverride[][] covers the plain (unshifted) symbol only; keys needing
// shift-aware handling (', \, =, -, ;, and the digit row) are handled by
// dedicated functions in keycode_output.h using the row/col constants below.
const uint8_t jisOverride[NUM_ROWS][NUM_COLS] = {
  /* pos0  */ {0, 0, 0, 0, KEY_TILDE}, // Caps position -> Zenkaku/Hankaku under JIS (0x35 is that toggle on a JIS-interpreting host, not a real tilde)
  /* pos1  */ {0, 0, 0, 0, 0},
  /* pos2  */ {0, 0, 0, 0, 0},
  /* pos3  */ {0, 0, 0, 0, 0},
  /* pos4  */ {0, 0, 0, 0, 0},
  /* pos5  */ {0, 0, 0, 0, 0},
  /* pos6  */ {0, 0, 0, 0, 0},
  /* pos7  */ {0, 0, 0, 0, 0},
  /* pos8  */ {0, 0, 0, 0, 0},
  /* pos9  */ {0, 0, 0, 0, 0},
  /* pos10 */ {0, 0, 0, 0, 0},
  /* pos11 */ {0, KEY_RIGHT_BRACE, 0, 0, 0},
  /* pos12 */ {0, KEY_BACKSLASH,  0, 0, 0}, // = and \ both handled specially
};

// Digit row (col 0) Shift+N fixes, JIS -> US target.
// code=0 means no override (1,3,4,5 already match between layouts).
// suppress=true: the target symbol is unshifted on JIS, so real Shift must
//   be let go for that one key. suppress=false: real Shift stays held and
//   we swap which digit/key goes with it.
// Indexed by position (0=Esc, 1="1", 2="2", ... 10="0", 11="-", 12="=").
const uint8_t digitShiftCode[NUM_ROWS]     = {0, 0, KEY_LEFT_BRACE, 0, 0, 0, KEY_EQUAL, KEY_6, KEY_QUOTE, KEY_8, KEY_9, 0, 0};
const bool    digitShiftSuppress[NUM_ROWS] = {0, 0, true,           0, 0, 0, true,      false, false,     false, false, 0, 0};

// --- Fn layer position map --------------------------------------------------
// Every FOO_ROW/FOO_COL pair below is a (row index, col index) into keymap[][].
const uint8_t FN_ROW = 12, FN_COL = 4;         // Fn key itself
const uint8_t MENU_ROW = 11, MENU_COL = 4;     // Fn+Menu toggles JIS mode
const uint8_t LSHIFT_ROW = 0, LSHIFT_COL = 3;
const uint8_t RSHIFT_ROW = 11, RSHIFT_COL = 3;

// Former dedicated Win key: base sends Caps (US) / Zenkaku-Hankaku (JIS, see
// jisOverride above). Fn+this position toggles "Mac mode" (see
// CTRL_L/CTRL_R below) instead of momentarily sending Win/Meta.
const uint8_t WIN_META_ROW = 0, WIN_META_COL = 4;

// Mac mode (Fn+Win toggles it): swaps the bottom-row Ctrl keys beside
// Space to send Cmd instead, for thumb-position parity with a real Mac
// keyboard. The Caps-position Ctrl (pos0, col2 above) is deliberately
// NOT swapped -- it stays genuine Ctrl even in Mac mode, since that's
// the position people actually want for Unix-style
// shortcuts (emacs bindings, terminal Ctrl-C, etc.) regardless of OS.
const uint8_t CTRL_L_ROW = 3, CTRL_L_COL = 4; // pos3/col4, "Ctrl" left of Space
const uint8_t CTRL_R_ROW = 9, CTRL_R_COL = 4; // pos9/col4, "Ctrl" right of Space

// JIS-fix-up positions (only meaningful if using the JIS feature above).
const uint8_t QUOTE_ROW = 11, QUOTE_COL = 2;
const uint8_t EQUAL_ROW = 12, EQUAL_COL = 0;
const uint8_t BACKSLASH_ROW = 8, BACKSLASH_COL = 4;   // exception position
const uint8_t MINUS_ROW = 11, MINUS_COL = 0;
const uint8_t SEMICOLON_ROW = 10, SEMICOLON_COL = 2;

// Fn layer: number row (col 0) -> F1-F12, in physical left-to-right order.
// Set an entry to 0 if that position has no F-key.
const uint8_t fnFunctionCode[NUM_ROWS] = {0, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
                                           KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12};

// Fn layer arrows: Fn+[ = up, Fn+' = right, Fn+; = left, Fn+/ = down.
const uint8_t ARROW_UP_ROW = 11, ARROW_UP_COL = 1;
const uint8_t ARROW_DOWN_ROW = 10, ARROW_DOWN_COL = 3;
const uint8_t ARROW_LEFT_ROW = 10, ARROW_LEFT_COL = 2;
const uint8_t ARROW_RIGHT_ROW = 11, ARROW_RIGHT_COL = 2;

// Fn layer: no physical ` ~ key on this board. Fn+Z = `, Fn+X = ~ -- two
// independent single-key bindings (not a Fn+Shift+key chord).
const uint8_t GRAVE_ROW = 1, GRAVE_COL = 3; // Fn+Z = `
const uint8_t TILDE_ROW = 2, TILDE_COL = 3; // Fn+X = ~

// Fn layer: navigation cluster, misc keys.
const uint8_t HOME_ROW = 8, HOME_COL = 2;
const uint8_t PGUP_ROW = 9, PGUP_COL = 2;
const uint8_t END_ROW = 8, END_COL = 3;
const uint8_t PGDN_ROW = 9, PGDN_COL = 3;
const uint8_t PRTSC_ROW = 8, PRTSC_COL = 1;
const uint8_t SCRLK_ROW = 9, SCRLK_COL = 1;
const uint8_t PAUSE_ROW = 10, PAUSE_COL = 1;
const uint8_t DELETE_ROW = 7, DELETE_COL = 4;   // exception position (Backspace)
const uint8_t INSERT_ROW = 8, INSERT_COL = 4;   // exception position (\)

// Fn layer media keys (Consumer Control usage page, not Keyboard).
const uint8_t VOLDOWN_ROW = 1, VOLDOWN_COL = 2;
const uint8_t VOLUP_ROW = 2, VOLUP_COL = 2;
const uint8_t MUTE_ROW = 3, MUTE_COL = 2;
const uint8_t PLAYPAUSE_ROW = 4, PLAYPAUSE_COL = 2;

// --- Onboard LED used as a JIS-mode indicator (optional) -------------------
#define JIS_LED_DDR  DDRB
#define JIS_LED_PORT PORTB
#define JIS_LED_BIT  PB0
#define JIS_LED_ACTIVE_LOW true

} // namespace BoardConfig
