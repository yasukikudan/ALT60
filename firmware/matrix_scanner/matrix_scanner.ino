#include <string.h>

// Pro Micro matrix diagnostic tool.
// Sweeps every pin as an output (LOW) one at a time while all other pins
// are INPUT_PULLUP, and reports which (out,in) pin pair goes LOW when a
// key is pressed. Works regardless of diode orientation since every
// ordered pair is tried.
//
// Prints two kinds of lines:
//   KEY DETECTED  out=Dx in=Dy   -- edge (open->closed) event
//   SNAPSHOT: ...                -- full raw state, once per second,
//                                   listing every pair that is closed
//                                   right now (or "none")

const uint8_t pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 14, 15, 16, 18, 19, 20, 21};
const uint8_t numPins = sizeof(pins) / sizeof(pins[0]);

bool closedState[numPins][numPins];
unsigned long lastSnapshot = 0;

void setAllInputPullup() {
  for (uint8_t i = 0; i < numPins; i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
}

void printPair(uint8_t i, uint8_t j) {
  Serial.print(F("D"));
  Serial.print(pins[i]);
  Serial.print(F("-D"));
  Serial.print(pins[j]);
  Serial.print(F(" "));
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }
  setAllInputPullup();
  memset(closedState, 0, sizeof(closedState));
  Serial.println(F("Matrix scanner ready. Press keys one at a time."));
}

void scanOnce(bool out[numPins][numPins]) {
  memset(out, 0, numPins * numPins * sizeof(bool));
  for (uint8_t i = 0; i < numPins; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
    delayMicroseconds(300);

    for (uint8_t j = 0; j < numPins; j++) {
      if (j == i) continue;
      out[i][j] = (digitalRead(pins[j]) == LOW);
    }

    pinMode(pins[i], INPUT_PULLUP);
    delayMicroseconds(100);
  }
}

void loop() {
  // Two independent full scans, a few ms apart. Only a pair that reads
  // closed on BOTH passes is trusted -- a transient noise glitch on a
  // floating pin almost never lines up on two separate sweeps, while a
  // real held-down key easily does.
  static bool scanA[numPins][numPins];
  static bool scanB[numPins][numPins];
  bool raw[numPins][numPins];

  scanOnce(scanA);
  delay(3);
  scanOnce(scanB);

  for (uint8_t i = 0; i < numPins; i++) {
    for (uint8_t j = 0; j < numPins; j++) {
      raw[i][j] = scanA[i][j] && scanB[i][j];
    }
  }

  // Edge-triggered reporting (one line per open->closed transition).
  for (uint8_t i = 0; i < numPins; i++) {
    for (uint8_t j = 0; j < numPins; j++) {
      if (j == i) continue;
      if (raw[i][j] && !closedState[i][j]) {
        closedState[i][j] = true;
        Serial.print(F("KEY DETECTED  out=D"));
        Serial.print(pins[i]);
        Serial.print(F("  in=D"));
        Serial.println(pins[j]);
      } else if (!raw[i][j] && closedState[i][j]) {
        closedState[i][j] = false;
      }
    }
  }

  // Full raw snapshot, once per second, so nothing is hidden by debounce
  // or edge logic.
  unsigned long now = millis();
  if (now - lastSnapshot >= 1000) {
    lastSnapshot = now;
    Serial.print(F("SNAPSHOT: "));
    bool any = false;
    for (uint8_t i = 0; i < numPins; i++) {
      for (uint8_t j = 0; j < numPins; j++) {
        if (j == i) continue;
        if (raw[i][j]) {
          printPair(i, j);
          any = true;
        }
      }
    }
    if (!any) Serial.print(F("(none)"));
    Serial.println();
  }
}
