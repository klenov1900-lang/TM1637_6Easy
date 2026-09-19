/*
 * TM1637_6Easy — QuickStart (6 digits, Chinese order)
 *
 * CLK = D8, DIO = D9, brightness = 0
 * Shows 123456 with a dot after the 3rd digit.
 *
 * Display: [1][2][3.][4][5][6]
 */

#include "TM1637_6Easy.h"

TM1637_6Easy display(8, 9, 0);            // CLK=8, DIO=9, brightness=0

void setup() {
    display.begin(1);                     // 6 digits, Chinese order
    display.showNumber(123456);
    display.setDotOn(2);                  // dot in digit 2  [1][2][3.][4][5][6]
    display.update();
    delay(2000);
}

void loop() {}
