#include "TM1637_6Easy.h"

// ============================================================
// КОНСТРУКТОР
// ============================================================

TM1637_6Easy::TM1637_6Easy(uint8_t clk_pin, uint8_t dio_pin, uint8_t brightness) {
    if (clk_pin == dio_pin) _errorHang();
    
    _clk_pin = clk_pin;
    _dio_pin = dio_pin;
    _brightness = (brightness > 7) ? 7 : brightness;
    _dots = 0x00;
    
    _clk_port = nullptr;
    _dio_port = nullptr;
    _clk_ddr = nullptr;
    _dio_ddr = nullptr;
    _clk_pin_reg = nullptr;
    _dio_pin_reg = nullptr;
    _clk_mask = 0;
    _dio_mask = 0;
    
    memset(_display_buffer, 0, DISPLAY_BUFFER_SIZE);
    _setOrder(0);
}

// ============================================================
// УСТАНОВКА ПОРЯДКА РАЗРЯДОВ
// ============================================================

void TM1637_6Easy::_setOrder(uint8_t mode) {
    if (mode == 0) {
        _display_order[0] = 0; _display_order[1] = 1; _display_order[2] = 2;
        _display_order[3] = 3; _display_order[4] = 4; _display_order[5] = 5;
    } else {
        _display_order[0] = 2; _display_order[1] = 1; _display_order[2] = 0;
        _display_order[3] = 5; _display_order[4] = 4; _display_order[5] = 3;
    }
}

// ============================================================
// АВАРИЙНЫЙ ОСТАНОВ
// ============================================================

void TM1637_6Easy::_errorHang() {
    DDRB |= (1 << 5);
    while(1) {
        PORTB |= (1 << 5);
        delay(50);
        PORTB &= ~(1 << 5);
        delay(50);
    }
}

// ============================================================
// ПРЯМОЕ ПОДКЛЮЧЕНИЕ К ПОРТАМ
// ============================================================

void TM1637_6Easy::_assignPin(uint8_t pin, volatile uint8_t* &port, 
                               volatile uint8_t* &ddr, volatile uint8_t* &pin_reg, 
                               uint8_t &mask) {
    if (pin == 0 || pin == 1) _errorHang();
    #if defined(A6)
    if (pin == A6) _errorHang();
    #endif
    #if defined(A7)
    if (pin == A7) _errorHang();
    #endif
    
    if (pin == 8)  { port = &PORTB; ddr = &DDRB; pin_reg = &PINB; mask = (1 << 0); }
    else if (pin == 9)  { port = &PORTB; ddr = &DDRB; pin_reg = &PINB; mask = (1 << 1); }
    else if (pin == 10) { port = &PORTB; ddr = &DDRB; pin_reg = &PINB; mask = (1 << 2); }
    else if (pin == 11) { port = &PORTB; ddr = &DDRB; pin_reg = &PINB; mask = (1 << 3); }
    else if (pin == 12) { port = &PORTB; ddr = &DDRB; pin_reg = &PINB; mask = (1 << 4); }
    else if (pin == 13) { port = &PORTB; ddr = &DDRB; pin_reg = &PINB; mask = (1 << 5); }
    
    else if (pin == 2)  { port = &PORTD; ddr = &DDRD; pin_reg = &PIND; mask = (1 << 2); }
    else if (pin == 3)  { port = &PORTD; ddr = &DDRD; pin_reg = &PIND; mask = (1 << 3); }
    else if (pin == 4)  { port = &PORTD; ddr = &DDRD; pin_reg = &PIND; mask = (1 << 4); }
    else if (pin == 5)  { port = &PORTD; ddr = &DDRD; pin_reg = &PIND; mask = (1 << 5); }
    else if (pin == 6)  { port = &PORTD; ddr = &DDRD; pin_reg = &PIND; mask = (1 << 6); }
    else if (pin == 7)  { port = &PORTD; ddr = &DDRD; pin_reg = &PIND; mask = (1 << 7); }
    
    #if defined(A0)
    else if (pin == A0) { port = &PORTC; ddr = &DDRC; pin_reg = &PINC; mask = (1 << 0); }
    #endif
    #if defined(A1)
    else if (pin == A1) { port = &PORTC; ddr = &DDRC; pin_reg = &PINC; mask = (1 << 1); }
    #endif
    #if defined(A2)
    else if (pin == A2) { port = &PORTC; ddr = &DDRC; pin_reg = &PINC; mask = (1 << 2); }
    #endif
    #if defined(A3)
    else if (pin == A3) { port = &PORTC; ddr = &DDRC; pin_reg = &PINC; mask = (1 << 3); }
    #endif
    #if defined(A4)
    else if (pin == A4) { port = &PORTC; ddr = &DDRC; pin_reg = &PINC; mask = (1 << 4); }
    #endif
    #if defined(A5)
    else if (pin == A5) { port = &PORTC; ddr = &DDRC; pin_reg = &PINC; mask = (1 << 5); }
    #endif
    
    else { _errorHang(); }
}

void TM1637_6Easy::_initPins() {
    _assignPin(_clk_pin, _clk_port, _clk_ddr, _clk_pin_reg, _clk_mask);
    _assignPin(_dio_pin, _dio_port, _dio_ddr, _dio_pin_reg, _dio_mask);
    
    if (_clk_port == nullptr || _dio_port == nullptr) _errorHang();
    
    *_clk_ddr |= _clk_mask;
    *_dio_ddr |= _dio_mask;
    *_clk_port &= ~_clk_mask;
    *_dio_port &= ~_dio_mask;
}

// ============================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================

void TM1637_6Easy::begin() { begin(0); }

void TM1637_6Easy::begin(uint8_t mode) {
    _setOrder(mode);
    _initPins();
    clear();
    setBrightness(_brightness);
}

// ============================================================
// РАБОТА С ПИНАМИ
// ============================================================

inline void TM1637_6Easy::_setPinHigh(volatile uint8_t* port, uint8_t mask) { *port |= mask; }
inline void TM1637_6Easy::_setPinLow(volatile uint8_t* port, uint8_t mask) { *port &= ~mask; }
inline bool TM1637_6Easy::_readPin(volatile uint8_t* pin_reg, uint8_t mask) { return (*pin_reg & mask) != 0; }

// ============================================================
// ПРОТОКОЛ TM1637
// ============================================================

inline void TM1637_6Easy::_delay() { __asm__("nop\n\t"); }

void TM1637_6Easy::_start() {
    _setPinHigh(_dio_port, _dio_mask);
    _setPinHigh(_clk_port, _clk_mask);
    _delay();
    _setPinLow(_dio_port, _dio_mask);
    _setPinLow(_clk_port, _clk_mask);
}

void TM1637_6Easy::_stop() {
    _setPinLow(_dio_port, _dio_mask);
    _setPinHigh(_clk_port, _clk_mask);
    _delay();
    _setPinHigh(_dio_port, _dio_mask);
}

bool TM1637_6Easy::_writeByte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        _setPinLow(_clk_port, _clk_mask);
        if (data & 0x01) _setPinHigh(_dio_port, _dio_mask);
        else _setPinLow(_dio_port, _dio_mask);
        _delay();
        _setPinHigh(_clk_port, _clk_mask);
        _delay();
        data >>= 1;
    }
    
    _setPinLow(_clk_port, _clk_mask);
    *_dio_ddr &= ~_dio_mask;
    _delay();
    _setPinHigh(_clk_port, _clk_mask);
    _delay();
    bool ack = !_readPin(_dio_pin_reg, _dio_mask);
    _setPinLow(_clk_port, _clk_mask);
    *_dio_ddr |= _dio_mask;
    
    return ack;
}

void TM1637_6Easy::_sendData() {
    uint8_t temp[DISPLAY_BUFFER_SIZE];
    memcpy(temp, _display_buffer, DISPLAY_BUFFER_SIZE);
    
    for (uint8_t i = 0; i < DISPLAY_BUFFER_SIZE; i++) {
        _display_buffer[i] = temp[_display_order[i]];
    }
    
    _start();
    _writeByte(0x40);
    _stop();
    
    _start();
    _writeByte(0xC0);
    for (uint8_t i = 0; i < DISPLAY_BUFFER_SIZE; i++) {
        uint8_t data = _display_buffer[i];
        if (_dots & (1 << _display_order[i])) data |= 0x80;
        _writeByte(data);
    }
    _stop();
}

// ============================================================
// ОСНОВНЫЕ ФУНКЦИИ
// ============================================================

void TM1637_6Easy::clear() {
    memset(_display_buffer, 0, DISPLAY_BUFFER_SIZE);
}

void TM1637_6Easy::setBrightness(uint8_t level) {
    _brightness = (level > 7) ? 7 : level;
    _start();
    _writeByte(0x88 | _brightness);
    _stop();
}

void TM1637_6Easy::update() {
    _sendData();
}

// ============================================================
// УПРАВЛЕНИЕ ТОЧКАМИ
// ============================================================

void TM1637_6Easy::setDotOn(uint8_t position) {
    if (position < DISPLAY_BUFFER_SIZE) _dots |= (1 << position);
}

void TM1637_6Easy::setDotOff(uint8_t position) {
    if (position < DISPLAY_BUFFER_SIZE) _dots &= ~(1 << position);
}

void TM1637_6Easy::clearDots() {
    _dots = 0x00;
}

void TM1637_6Easy::setDots(uint8_t dots) {
    _dots = dots & 0x3F;
}

// ============================================================
// showNumber - ВЫВОД ЦЕЛЫХ ЧИСЕЛ
// ============================================================

void TM1637_6Easy::showNumber(int32_t number) {
    if (number < -99999) number = -99999;
    if (number > 999999) number = 999999;
    
    uint8_t digits[DISPLAY_BUFFER_SIZE];
    for (int i = 0; i < DISPLAY_BUFFER_SIZE; i++) digits[i] = 20;
    
    bool negative = (number < 0);
    uint32_t absVal = abs(number);
    
    int i = DISPLAY_BUFFER_SIZE - 1;
    while (absVal > 0 && i >= 0) {
        digits[i--] = absVal % 10;
        absVal /= 10;
    }
    
    if (absVal == 0 && i == DISPLAY_BUFFER_SIZE - 1) digits[DISPLAY_BUFFER_SIZE - 1] = 0;
    if (negative && i >= 0) digits[i] = 19;
    
    clear();
    for (int pos = 0; pos < DISPLAY_BUFFER_SIZE; pos++) {
        if (digits[pos] < SEGMENTS_TABLE_SIZE) {
            _display_buffer[pos] = pgm_read_byte(&SEGMENTS[digits[pos]]);
        }
    }
}