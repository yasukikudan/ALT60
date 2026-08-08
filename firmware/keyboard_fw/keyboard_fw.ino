#include "board_config.h"
#include "matrix_scan.h"
#include "keycode_output.h"

const unsigned long CHATTER_REPORT_INTERVAL_MS = 2000;

void setup() {
  Serial.begin(9600); // only used if a terminal is actually connected -- see printChatterStats()
  MatrixScan::begin();
  KeycodeOutput::begin();
}

void loop() {
  static unsigned long lastScan = 0;
  static unsigned long lastChatterReport = 0;
  unsigned long now = millis();

  if (now - lastScan >= BoardConfig::SCAN_INTERVAL_MS) {
    lastScan = now;
    MatrixScan::scan(KeycodeOutput::handleKeyEvent);
    KeycodeOutput::refreshJisLed();
  }

  if (now - lastChatterReport >= CHATTER_REPORT_INTERVAL_MS) {
    lastChatterReport = now;
    MatrixScan::printChatterStats();
  }
}
