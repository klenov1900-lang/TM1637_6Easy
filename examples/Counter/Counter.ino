/*
 * TM1637_6Easy — Counter
 *
 * Counts seconds from 0 to 99 and displays them.
 * CLK = D8, DIO = D9, brightness = 3
 */

#include "TM1637_6Easy.h"

TM1637_6Easy display(8, 9, 3);

void setup() {
    display.begin();                      // 6 digits, normal order
    display.showNumber(0);
    display.update();
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last >= 1000) {
        last = millis();
        display.showNumber((millis() / 1000) % 100);
        display.update();
    }
}
