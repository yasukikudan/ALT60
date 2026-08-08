#pragma once
#include <Arduino.h>
#include "board_config.h"

// Pure input layer: knows nothing about keycodes. Scans the physical matrix
// described in board_config.h, debounces it, and calls onKeyEvent(row, col,
// pressed) once per confirmed state change. Nothing in this file should
// need editing to adapt to a different board -- see board_config.h instead.

namespace MatrixScan {

using namespace BoardConfig;

bool keyState[NUM_ROWS][NUM_COLS];
bool lastRaw[NUM_ROWS][NUM_COLS];
unsigned long lastChangeTime[NUM_ROWS][NUM_COLS];

// Chatter stats: a "chatter" is a press that lands within CHATTER_WINDOW_MS
// of that same position's previous release -- almost certainly the same
// physical actuation re-triggering, not a deliberate fast re-press.
unsigned long lastReleaseTime[NUM_ROWS][NUM_COLS];
uint16_t chatterCount[NUM_ROWS][NUM_COLS];
uint16_t pressCount[NUM_ROWS][NUM_COLS]; // total real presses, so chatter can be reported as a rate, not a raw count skewed by how often a key is typed

void begin() {
  for (uint8_t c = 0; c < NUM_COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }
}

// Sweeps the whole matrix once, invoking onKeyEvent(row, col, pressed) for
// every position whose debounced state just changed. If PRIORITY_COL_A/B
// (board_config.h) name real columns, those fire first on every pass, then
// everything else in scan order.
template <typename EventHandler>
void scan(EventHandler onKeyEvent) {
  unsigned long now = millis();

  const uint8_t MAX_EVENTS = NUM_ROWS * NUM_COLS;
  uint8_t evR[MAX_EVENTS], evC[MAX_EVENTS];
  bool evRaw[MAX_EVENTS];
  uint8_t evCount = 0;

  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(30);

    for (uint8_t c = 0; c < NUM_COLS; c++) {
      bool raw = (digitalRead(colPins[c]) == LOW);

      if (raw != lastRaw[r][c]) {
        lastRaw[r][c] = raw;
        lastChangeTime[r][c] = now;
      }

      if ((now - lastChangeTime[r][c]) >= DEBOUNCE_MS && keyState[r][c] != raw) {
        keyState[r][c] = raw;
        if (raw) {
          pressCount[r][c]++;
          if (lastReleaseTime[r][c] != 0 && (now - lastReleaseTime[r][c]) < CHATTER_WINDOW_MS) {
            chatterCount[r][c]++;
          }
        } else {
          lastReleaseTime[r][c] = now;
        }
        if (evCount < MAX_EVENTS) {
          evR[evCount] = r;
          evC[evCount] = c;
          evRaw[evCount] = raw;
          evCount++;
        }
      }
    }

    pinMode(rowPins[r], INPUT_PULLUP);
  }

  for (uint8_t i = 0; i < evCount; i++) {
    if (evC[i] == PRIORITY_COL_A || evC[i] == PRIORITY_COL_B) {
      onKeyEvent(evR[i], evC[i], evRaw[i]);
    }
  }
  for (uint8_t i = 0; i < evCount; i++) {
    if (evC[i] != PRIORITY_COL_A && evC[i] != PRIORITY_COL_B) {
      onKeyEvent(evR[i], evC[i], evRaw[i]);
    }
  }
}

// Prints every position with a nonzero chatter count, as a RATE (chatters
// per real press, as a percentage) rather than a raw count -- a raw count
// is dominated by how often a key gets typed, not by how flaky its
// connection actually is. Safe to call every loop() even with nothing
// connected: guarded by `if (Serial)`, which is only true once a host has
// actually opened the port -- unlike blindly calling Serial.print(), this
// can't block forever and freeze the keyboard when no one's listening.
void printChatterStats() {
  if (!Serial) return;
  bool any = false;
  for (uint8_t r = 0; r < NUM_ROWS; r++) {
    for (uint8_t c = 0; c < NUM_COLS; c++) {
      if (chatterCount[r][c] > 0 && pressCount[r][c] > 0) {
        uint16_t ratePct = (uint32_t)chatterCount[r][c] * 100 / pressCount[r][c];
        Serial.print(F("D"));
        Serial.print(rowPins[r]);
        Serial.print(F("/D"));
        Serial.print(colPins[c]);
        Serial.print(F(": "));
        Serial.print(chatterCount[r][c]);
        Serial.print(F(" chatters / "));
        Serial.print(pressCount[r][c]);
        Serial.print(F(" presses ("));
        Serial.print(ratePct);
        Serial.println(F("%)"));
        any = true;
      }
    }
  }
  if (!any) Serial.println(F("(no chatter recorded)"));
}

} // namespace MatrixScan
