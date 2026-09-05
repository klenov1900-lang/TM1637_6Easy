#include "TM1637_6Easy.h"

TM1637_6Easy display(8, 9, 3);

void setup() {
    display.begin();
    
    display.showNumber(2026);
    display.update();
    delay(2000);
    
    display.showNumber(123456);
    display.setDots(0b00010100);
    display.update();
    delay(2000);
    display.clearDots();
    
    display.showNumber(123456);
    display.setDotOn(1);
    display.setDotOn(3);
    display.update();
    delay(2000);
    display.clearDots();
    
    display.clear();
    display.setDots(0b00111111);
    display.update();
    delay(2000);
    
    display.clearDots();
    display.update();
    delay(1000);
}

void loop() {
    static uint32_t last = 0;
    static bool dot_state = false;
    
    if (millis() - last >= 1000) {
        last = millis();
        
        display.showNumber((millis() / 1000) % 100);
        
        if (dot_state) {
            display.setDots(0b00010000);
        } else {
            display.clearDots();
        }
        dot_state = !dot_state;
        
        display.update();
    }
}