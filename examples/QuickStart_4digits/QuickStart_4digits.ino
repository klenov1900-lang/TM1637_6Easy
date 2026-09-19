/*
 * TM1637_6Easy — QuickStart (4 digits)
 *
 * CLK = D7, DIO = D6, brightness = 0
 * Shows 1234 with a dot after the 3rd digit.
 *
 * Display: [1][2][3.][4]
 */

#include "TM1637_6Easy.h"

TM1637_6Easy display(7, 6, 0);            // CLK=7, DIO=6, brightness=0

void setup() {
    display.begin(2);                     // 4 digits
    display.showNumber(1234);
    display.setDotOn(2);                  // dot in digit 2 [1][2][3.][4]
    display.update();
    delay(2000);
}

void loop() {}
