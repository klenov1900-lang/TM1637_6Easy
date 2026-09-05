# TM1637_6Easy

High-performance library for 6-digit LED displays based on TM1637 driver with direct port I/O.

## Features

- Direct port I/O for maximum speed
- Minimal memory footprint
- Supports any digital pins D2-D13, A0-A5
- Display numbers with automatic leading zero removal
- Dot control (colon)
- Two digit orders: normal and "Chinese" (2,1,0,5,4,3)

## Installation

1. Download the library
2. Place in `libraries/TM1637_6Easy`
3. Restart Arduino IDE

## Quick Start

```cpp
#include "TM1637_6Easy.h"

TM1637_6Easy display(8, 9, 3);

void setup() {
    display.begin();
    display.showNumber(2026);
    display.update();
}

void loop() {}
