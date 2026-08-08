#pragma once
#include "HID-Project.h"
#include <EEPROM.h>
#include "board_config.h"

// Pure output layer: knows nothing about pin numbers or matrix scanning.
// Given (row, col, pressed) events from MatrixScan, decides what to send
// to the host over USB HID, using the keymap/positions in board_config.h.
// Nothing in this file should need editing to adapt to a different board.

namespace KeycodeOutput {

using namespace BoardConfig;

bool fnHeld = false;
bool jisMode = false;
bool shiftHeld = false;
uint8_t activeShiftCode = KEY_LEFT_SHIFT;

// Optional onboard LED as a JIS-mode indicator (pin picked in
// board_config.h). If your board has no spare LED, these two functions are
// harmless no-op-ish writes to whatever pin you pointed them at; feel free
// to gut them.
void setJisLed(bool on) {
  JIS_LED_DDR |= (1 << JIS_LED_BIT);
  bool level = JIS_LED_ACTIVE_LOW ? !on : on;
  if (level) {
    JIS_LED_PORT |= (1 << JIS_LED_BIT);
  } else {
    JIS_LED_PORT &= ~(1 << JIS_LED_BIT);
  }
}

const int JIS_MODE_EEPROM_ADDR = 0;

// Re-applies the LED every call rather than trusting it to stay put -- the
// USB stack blinks RX/TX LEDs for its own activity indication on some
// boards/cores and can stomp on our state, especially right after boot
// before USB enumerates.
void refreshJisLed() {
  setJisLed(jisMode);
}

void begin() {
  NKROKeyboard.begin();
  // JIS mode is the default. EEPROM only ever stores 0 (explicitly US) or 1
  // (explicitly JIS); unwritten flash reads 0xFF, so anything other than a
  // literal 0 is treated as "JIS" -- a brand new/erased board starts in
  // JIS mode without needing Fn+Menu pressed once first.
  jisMode = (EEPROM.read(JIS_MODE_EEPROM_ADDR) != 0);
  setJisLed(jisMode);
}

void sendKey(uint8_t code, bool pressed) {
  if (code == 0) return;
  if (pressed) {
    NKROKeyboard.press((KeyboardKeycode)code);
  } else {
    NKROKeyboard.release((KeyboardKeycode)code);
  }
}

// Returns true if it fully handled the event (caller should not fall
// through to the plain keymap lookup).
bool handleFnAndMenu(uint8_t r, uint8_t c, bool pressed) {
  if (r == FN_ROW && c == FN_COL) {
    fnHeld = pressed;
    return true;
  }
  if (r == MENU_ROW && c == MENU_COL && fnHeld) {
    if (pressed) {
      jisMode = !jisMode;
      setJisLed(jisMode);
      EEPROM.update(JIS_MODE_EEPROM_ADDR, jisMode ? 1 : 0);
    }
    return true;
  }
  return false;
}

// No physical ` ~ key on this board. Fn+Z = ` (grave), Fn+X = ~ (tilde) --
// two independent single-key bindings, not a shared Fn+Shift+key chord
// (that was an awkward 3-key stretch). Each sends its own synthetic Shift
// as needed, so neither depends on the user's real Shift state.
// US mode: unshifted KEY_TILDE = `, synthetic-Shift+KEY_TILDE = ~.
// JIS mode: plain KEY_TILDE lands on the hankaku/zenkaku toggle instead (no
// character) on a Japanese-layout host, so we use the JIS "@" key (shifted
// = `) and the JIS "^" key (shifted = ~), i.e. Shift+LEFT_BRACE / Shift+EQUAL.
//
// Our synthetic Shift always uses KEY_LEFT_SHIFT. If the user's real Shift
// is also Left Shift and is genuinely held, releasing KEY_LEFT_SHIFT here
// would cut their real Shift too (NKRO tracks by keycode, not by who
// pressed it) -- so only release it when that's safe.
void releaseSyntheticShift() {
  if (!(shiftHeld && activeShiftCode == KEY_LEFT_SHIFT)) {
    NKROKeyboard.release(KEY_LEFT_SHIFT);
  }
}

bool handleFnGraveKeys(uint8_t r, uint8_t c, bool pressed) {
  if (!fnHeld) return false;
  bool isGrave = (r == GRAVE_ROW && c == GRAVE_COL);
  bool isTilde = (r == TILDE_ROW && c == TILDE_COL);
  if (!isGrave && !isTilde) return false;

  if (isGrave && !jisMode) {
    sendKey(KEY_TILDE, pressed);
  } else if (isGrave && jisMode) {
    if (pressed) {
      NKROKeyboard.press(KEY_LEFT_SHIFT);
      NKROKeyboard.press(KEY_LEFT_BRACE);
    } else {
      NKROKeyboard.release(KEY_LEFT_BRACE);
      releaseSyntheticShift();
    }
  } else { // isTilde
    uint8_t symbol = jisMode ? KEY_EQUAL : KEY_TILDE;
    if (pressed) {
      NKROKeyboard.press(KEY_LEFT_SHIFT);
      NKROKeyboard.press((KeyboardKeycode)symbol);
    } else {
      NKROKeyboard.release((KeyboardKeycode)symbol);
      releaseSyntheticShift();
    }
  }
  return true;
}

bool handleFnFunctionRow(uint8_t r, uint8_t c, bool pressed) {
  if (!(fnHeld && c == 0 && fnFunctionCode[r] != 0)) return false;
  sendKey(fnFunctionCode[r], pressed);
  return true;
}

bool handleFnArrows(uint8_t r, uint8_t c, bool pressed) {
  if (!fnHeld) return false;
  if (r == ARROW_UP_ROW && c == ARROW_UP_COL)       { sendKey(KEY_UP_ARROW, pressed);    return true; }
  if (r == ARROW_LEFT_ROW && c == ARROW_LEFT_COL)   { sendKey(KEY_LEFT_ARROW, pressed);  return true; }
  if (r == ARROW_RIGHT_ROW && c == ARROW_RIGHT_COL) { sendKey(KEY_RIGHT_ARROW, pressed); return true; }
  if (r == ARROW_DOWN_ROW && c == ARROW_DOWN_COL)   { sendKey(KEY_DOWN_ARROW, pressed);  return true; }
  return false;
}

bool handleFnNavigation(uint8_t r, uint8_t c, bool pressed) {
  if (!fnHeld) return false;
  if (r == HOME_ROW && c == HOME_COL)   { sendKey(KEY_HOME, pressed);     return true; }
  if (r == PGUP_ROW && c == PGUP_COL)   { sendKey(KEY_PAGE_UP, pressed);  return true; }
  if (r == END_ROW && c == END_COL)     { sendKey(KEY_END, pressed);     return true; }
  if (r == PGDN_ROW && c == PGDN_COL)   { sendKey(KEY_PAGE_DOWN, pressed); return true; }
  if (r == PRTSC_ROW && c == PRTSC_COL) { sendKey(KEY_PRINTSCREEN, pressed); return true; }
  if (r == SCRLK_ROW && c == SCRLK_COL) { sendKey(KEY_SCROLL_LOCK, pressed); return true; }
  if (r == PAUSE_ROW && c == PAUSE_COL) { sendKey(KEY_PAUSE, pressed);    return true; }
  if (r == DELETE_ROW && c == DELETE_COL) { sendKey(KEY_DELETE, pressed); return true; }
  if (r == INSERT_ROW && c == INSERT_COL) { sendKey(KEY_INSERT, pressed); return true; }
  return false;
}

// Media/volume keys, via the Consumer Control HID usage page (not the
// keyboard page -- the OS handles these as system volume, not keypresses).
bool handleFnMedia(uint8_t r, uint8_t c, bool pressed) {
  if (!fnHeld) return false;
  if (r == VOLDOWN_ROW && c == VOLDOWN_COL) {
    if (pressed) Consumer.press(MEDIA_VOLUME_DOWN); else Consumer.release(MEDIA_VOLUME_DOWN);
    return true;
  }
  if (r == VOLUP_ROW && c == VOLUP_COL) {
    if (pressed) Consumer.press(MEDIA_VOLUME_UP); else Consumer.release(MEDIA_VOLUME_UP);
    return true;
  }
  if (r == MUTE_ROW && c == MUTE_COL) {
    if (pressed) Consumer.press(MEDIA_VOLUME_MUTE); else Consumer.release(MEDIA_VOLUME_MUTE);
    return true;
  }
  if (r == PLAYPAUSE_ROW && c == PLAYPAUSE_COL) {
    if (pressed) Consumer.press(MEDIA_PLAY_PAUSE); else Consumer.release(MEDIA_PLAY_PAUSE);
    return true;
  }
  return false;
}

// Former dedicated Win key: base keymap entry sends Caps/Zenkaku-Hankaku
// (see board_config.h), but Fn+this position sends the real Win/Meta key
// instead, regardless of jisMode.
bool handleFnMeta(uint8_t r, uint8_t c, bool pressed) {
  if (!(fnHeld && r == WIN_META_ROW && c == WIN_META_COL)) return false;
  sendKey(KEY_LEFT_GUI, pressed);
  return true;
}

// Handles Left/Right Shift's own key position directly (sends its real HID
// press/release itself) instead of leaving it to the generic fallback, so
// that by the time correctGraveTildeIfNeeded() runs, Shift is *actually*
// active at the HID level -- not just in our shiftHeld bookkeeping.
bool trackShiftState(uint8_t r, uint8_t c, bool pressed) {
  if (r == LSHIFT_ROW && c == LSHIFT_COL) {
    sendKey(KEY_LEFT_SHIFT, pressed);
    shiftHeld = pressed;
    if (pressed) activeShiftCode = KEY_LEFT_SHIFT;
    return true;
  }
  if (r == RSHIFT_ROW && c == RSHIFT_COL) {
    sendKey(KEY_RIGHT_SHIFT, pressed);
    shiftHeld = pressed;
    if (pressed) activeShiftCode = KEY_RIGHT_SHIFT;
    return true;
  }
  return false;
}

// --- JIS compensation handlers ---------------------------------------------
// These only ever match if board_config.h's *_ROW/*_COL constants point at
// real keys and jisMode is on. On a board that doesn't need JIS
// compensation, jisMode never becomes meaningfully true in practice (or you
// can simply never press Fn+Menu) and these are all silent no-ops.

bool handleDigitShiftJis(uint8_t r, uint8_t c, bool pressed) {
  if (!(jisMode && c == 0 && shiftHeld && digitShiftCode[r] != 0)) return false;

  uint8_t oc = digitShiftCode[r];
  if (digitShiftSuppress[r]) {
    if (pressed) {
      NKROKeyboard.release((KeyboardKeycode)activeShiftCode);
      NKROKeyboard.press((KeyboardKeycode)oc);
    } else {
      NKROKeyboard.release((KeyboardKeycode)oc);
      NKROKeyboard.press((KeyboardKeycode)activeShiftCode);
    }
  } else {
    sendKey(oc, pressed);
  }
  return true;
}

// ' has no direct key on a JIS layout.
// Unshifted (') = synthetic Shift+7. Shifted (") = real Shift (already
// held) + 2, since JIS Shift+2 produces ".
bool handleQuoteJis(uint8_t r, uint8_t c, bool pressed) {
  if (!(jisMode && r == QUOTE_ROW && c == QUOTE_COL)) return false;

  uint8_t k = shiftHeld ? KEY_2 : KEY_7;
  if (!shiftHeld) {
    sendKey(KEY_LEFT_SHIFT, pressed);
  }
  sendKey(k, pressed);
  return true;
}

// \ (unshifted) = the JIS "ro" key (INTERNATIONAL1).
// Shift+\ = | (pipe) = Shift + the JIS yen key (INTERNATIONAL3), not
// Shift+INTERNATIONAL1 (that gives _).
bool handleBackslashJis(uint8_t r, uint8_t c, bool pressed) {
  if (!(jisMode && r == BACKSLASH_ROW && c == BACKSLASH_COL)) return false;

  uint8_t k = shiftHeld ? KEY_INTERNATIONAL3 : KEY_INTERNATIONAL1;
  sendKey(k, pressed);
  return true;
}

// - (unshifted) already matches between layouts. Shift+- should be _
// (underscore) on US, but plain Shift+MINUS lands on = under JIS -- so use
// Shift + the JIS "ro" key (INTERNATIONAL1) instead, which is _ shifted.
bool handleMinusJis(uint8_t r, uint8_t c, bool pressed) {
  if (!(jisMode && r == MINUS_ROW && c == MINUS_COL && shiftHeld)) return false;
  sendKey(KEY_INTERNATIONAL1, pressed); // _
  return true;
}

// ; (unshifted) already matches between layouts. Shift+; should be : on
// US, but plain Shift+SEMICOLON lands on + under JIS -- so let go of real
// Shift and send plain QUOTE instead (unshifted QUOTE = : under JIS).
bool handleSemicolonJis(uint8_t r, uint8_t c, bool pressed) {
  if (!(jisMode && r == SEMICOLON_ROW && c == SEMICOLON_COL && shiftHeld)) return false;
  if (pressed) {
    NKROKeyboard.release((KeyboardKeycode)activeShiftCode);
    NKROKeyboard.press(KEY_QUOTE);
  } else {
    NKROKeyboard.release(KEY_QUOTE);
    NKROKeyboard.press((KeyboardKeycode)activeShiftCode);
  }
  return true;
}

// = is Shift+- (the minus key) on a JIS layout.
// + (shifted =) = real Shift (already held) + ; , since JIS Shift+; is +.
bool handleEqualJis(uint8_t r, uint8_t c, bool pressed) {
  if (!(jisMode && r == EQUAL_ROW && c == EQUAL_COL)) return false;

  if (shiftHeld) {
    sendKey(KEY_SEMICOLON, pressed);
  } else {
    sendKey(KEY_LEFT_SHIFT, pressed);
    sendKey(KEY_MINUS, pressed);
  }
  return true;
}

// Called once per debounced (row, col) state change from MatrixScan.
void handleKeyEvent(uint8_t r, uint8_t c, bool pressed) {
  if (handleFnAndMenu(r, c, pressed)) return;
  if (handleFnGraveKeys(r, c, pressed)) return;
  if (handleFnFunctionRow(r, c, pressed)) return;
  if (handleFnArrows(r, c, pressed)) return;
  if (handleFnNavigation(r, c, pressed)) return;
  if (handleFnMedia(r, c, pressed)) return;
  if (handleFnMeta(r, c, pressed)) return;

  if (trackShiftState(r, c, pressed)) return;

  if (handleDigitShiftJis(r, c, pressed)) return;
  if (handleMinusJis(r, c, pressed)) return;
  if (handleSemicolonJis(r, c, pressed)) return;
  if (handleQuoteJis(r, c, pressed)) return;
  if (handleBackslashJis(r, c, pressed)) return;
  if (handleEqualJis(r, c, pressed)) return;

  // Fn is held but this position has no Fn-layer binding -- swallow it
  // instead of falling through to its normal (non-Fn) keycode.
  if (fnHeld) return;

  uint8_t code = keymap[r][c];
  if (jisMode && jisOverride[r][c] != 0) {
    code = jisOverride[r][c];
  }
  sendKey(code, pressed);
}

} // namespace KeycodeOutput
