/*
 * TM1637_6Easy — Two displays at once
 *
 * 6-digit display on D8/D9 (brightness 5)
 * 4-digit display on D7/D6 (brightness 3)
 *
 * Both show 1234.
 */

#include "TM1637_6Easy.h"

TM1637_6Easy display6(8, 9, 5);           // CLK=8, DIO=9, brightness=5
TM1637_6Easy display4(7, 6, 3);           // CLK=7, DIO=6, brightness=3

void setup() {
    display6.begin(0);                    // 6 digits, normal order
    display6.showNumber(1234);
    display6.update();

    display4.begin(2);                    // 4 digits
    display4.showNumber(1234);
    display4.update();
}

void loop() {}
