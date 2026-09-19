#include "TM1637_6Easy.h"

// ============================================================
// ГРУБАЯ ЗАДЕРЖКА ДЛЯ _errorHang
// Не зависит от таймеров, работает до init() Arduino.
// ~1 мс на 1 единицу при 16 МГц (керамика — примерно так же).
// ============================================================

inline void _crudeDelay(uint16_t ms) {
    while (ms--) {
        for (volatile uint16_t i = 0; i < 2000; i++) {
            __asm__ volatile ("nop");
        }
    }
}

// ============================================================
// СТРУКТУРА ДЛЯ ТАБЛИЦЫ ПИНОВ
// ============================================================

struct PinRegisterInfo {
    volatile uint8_t* port;
    volatile uint8_t* ddr;
    volatile uint8_t* pin_reg;
    uint8_t mask;
};

// ============================================================
// ТАБЛИЦА ПИНОВ ДЛЯ ATmega328P (Arduino Uno/Nano)
// ============================================================

static const PinRegisterInfo PIN_MAP[] PROGMEM = {
    {nullptr, nullptr, nullptr, 0},     // 0
    {nullptr, nullptr, nullptr, 0},     // 1
    {&PORTD, &DDRD, &PIND, (1 << 2)},   // 2
    {&PORTD, &DDRD, &PIND, (1 << 3)},   // 3
    {&PORTD, &DDRD, &PIND, (1 << 4)},   // 4
    {&PORTD, &DDRD, &PIND, (1 << 5)},   // 5
    {&PORTD, &DDRD, &PIND, (1 << 6)},   // 6
    {&PORTD, &DDRD, &PIND, (1 << 7)},   // 7
    {&PORTB, &DDRB, &PINB, (1 << 0)},   // 8
    {&PORTB, &DDRB, &PINB, (1 << 1)},   // 9
    {&PORTB, &DDRB, &PINB, (1 << 2)},   // 10
    {&PORTB, &DDRB, &PINB, (1 << 3)},   // 11
    {&PORTB, &DDRB, &PINB, (1 << 4)},   // 12
    {&PORTB, &DDRB, &PINB, (1 << 5)},   // 13
    #if defined(A0)
    {&PORTC, &DDRC, &PINC, (1 << 0)},   // 14 (A0)
    #endif
    #if defined(A1)
    {&PORTC, &DDRC, &PINC, (1 << 1)},   // 15 (A1)
    #endif
    #if defined(A2)
    {&PORTC, &DDRC, &PINC, (1 << 2)},   // 16 (A2)
    #endif
    #if defined(A3)
    {&PORTC, &DDRC, &PINC, (1 << 3)},   // 17 (A3)
    #endif
    #if defined(A4)
    {&PORTC, &DDRC, &PINC, (1 << 4)},   // 18 (A4)
    #endif
    #if defined(A5)
    {&PORTC, &DDRC, &PINC, (1 << 5)},   // 19 (A5)
    #endif
};

// ============================================================
// КОНСТРУКТОР
// ============================================================

TM1637_6Easy::TM1637_6Easy(uint8_t clk_pin, uint8_t dio_pin, uint8_t brightness) {
    if (clk_pin == dio_pin) _errorHang();
    
    _clk_pin = clk_pin;
    _dio_pin = dio_pin;
    _brightness = (brightness > 7) ? 7 : brightness;
    _dots = 0x00;
    _digits_count = 6;
    
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
// АВАРИЙНЫЙ ОСТАНОВ (фатальные ошибки)
// Мигание встроенного светодиода PB5 (~3 Гц).
// Не зависит от таймеров — работает до init() Arduino.
// ============================================================

void TM1637_6Easy::_errorHang() {
    DDRB |= (1 << 5);
    while(1) {
        PORTB |= (1 << 5);
        _crudeDelay(166);
        PORTB &= ~(1 << 5);
        _crudeDelay(166);
    }
}

// ============================================================
// ОПРЕДЕЛЕНИЕ ПИНОВ (ЧЕРЕЗ ТАБЛИЦУ)
// ============================================================

void TM1637_6Easy::_assignPin(uint8_t pin, volatile uint8_t* &port, 
                               volatile uint8_t* &ddr, volatile uint8_t* &pin_reg, 
                               uint8_t &mask) {
    if (pin < 2 || pin > 19) {
        _errorHang();
        return;
    }
    
    #if defined(A6)
    if (pin == A6 || pin == A7) {
        _errorHang();
        return;
    }
    #endif

    PinRegisterInfo info;
    memcpy_P(&info, &PIN_MAP[pin], sizeof(PinRegisterInfo));
    
    port = info.port;
    ddr = info.ddr;
    pin_reg = info.pin_reg;
    mask = info.mask;
}

// ============================================================
// ИНИЦИАЛИЗАЦИЯ ПИНОВ
// ============================================================

void TM1637_6Easy::_initPins() {
    _assignPin(_clk_pin, _clk_port, _clk_ddr, _clk_pin_reg, _clk_mask);
    _assignPin(_dio_pin, _dio_port, _dio_ddr, _dio_pin_reg, _dio_mask);
    
    if (_clk_port == nullptr || _dio_port == nullptr) {
        _errorHang();
    }
    
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
    if (mode == 2) {
        _digits_count = 4;
        _display_order[0] = 0;
        _display_order[1] = 1;
        _display_order[2] = 2;
        _display_order[3] = 3;
    } else {
        _digits_count = 6;
        _setOrder(mode);
    }
    
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
// ЗАДЕРЖКА (1 nop)
// ============================================================

inline void TM1637_6Easy::_delay() {
    __asm__ volatile ("nop\n\t");
}

// ============================================================
// ПРОТОКОЛ TM1637
// ============================================================

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
        _delay();
        if (data & 0x01) {
            _setPinHigh(_dio_port, _dio_mask);
        } else {
            _setPinLow(_dio_port, _dio_mask);
        }
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
    _delay();
    *_dio_ddr |= _dio_mask;
    
    return ack;
}

// ============================================================
// ОТПРАВКА ДАННЫХ
// Точки применяются здесь, а не в _setPositionBuffer.
// ============================================================

void TM1637_6Easy::_sendData() {
    _start();
    _writeByte(0x40);
    _stop();
    
    _start();
    _writeByte(0xC0);
    for (uint8_t i = 0; i < _digits_count; i++) {
        uint8_t idx = _display_order[i];
        uint8_t data = _display_buffer[idx];
        if (_dots & (1 << idx)) {
            data |= 0x80;
        }
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
    if (position < _digits_count) _dots |= (1 << position);
}

void TM1637_6Easy::setDotOff(uint8_t position) {
    if (position < _digits_count) _dots &= ~(1 << position);
}

void TM1637_6Easy::clearDots() {
    _dots = 0x00;
}

void TM1637_6Easy::setDots(uint8_t dots) {
    _dots = dots & ((1 << _digits_count) - 1);
}

// ============================================================
// УПРАВЛЕНИЕ ПИТАНИЕМ
// ============================================================

void TM1637_6Easy::displayOff() {
    _start();
    _writeByte(0x80);
    _stop();
}

void TM1637_6Easy::displayOn() {
    _start();
    _writeByte(0x88 | _brightness);
    _stop();
}

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================

uint8_t TM1637_6Easy::_encodeChar(uint8_t symbol) {
    if (symbol >= SEGMENTS_TABLE_SIZE) return 20;
    return pgm_read_byte(&SEGMENTS[symbol]);
}

// Точки в буфер здесь НЕ добавляются — они применяются в _sendData.
void TM1637_6Easy::_setPositionBuffer(uint8_t position, uint8_t symbol) {
    if (position >= DISPLAY_BUFFER_SIZE) return;
    if (symbol >= SEGMENTS_TABLE_SIZE) symbol = 20;
    _display_buffer[position] = _encodeChar(symbol);
}

// ============================================================
// showNumber
// ВНИМАНИЕ: не вызывает update()!
// Это позволяет вручную управлять точками через setDotOn/setDotOff,
// а затем отправить всё одной командой display.update().
//
// Диапазоны:
//   4 разряда:  -999 .. 9999
//   6 разрядов: -99999 .. 999999
// Вне диапазона — выводит "Err".
// ============================================================

void TM1637_6Easy::showNumber(int32_t number) {
    int32_t max_val, min_val;
    if (_digits_count == 4) {
        max_val = 9999;
        min_val = -999;
    } else {
        max_val = 999999;
        min_val = -99999;
    }
    
    clear();
    
    if (number < min_val || number > max_val) {
        uint8_t pos = 0;
        if (number < 0) {
            _setPositionBuffer(pos++, 19);   // минус
        }
        _setPositionBuffer(pos++, 14);       // 'E'
        _setPositionBuffer(pos++, 18);       // 'r'
        _setPositionBuffer(pos,   18);       // 'r'
        return;
    }
    
    uint8_t digits[DISPLAY_BUFFER_SIZE];
    for (int i = 0; i < _digits_count; i++) digits[i] = 20;   // пробел
    
    bool negative = (number < 0);
    uint32_t absVal = abs(number);
    
    int i = _digits_count - 1;
    while (absVal > 0 && i >= 0) {
        digits[i--] = absVal % 10;
        absVal /= 10;
    }
    
    if (absVal == 0 && i == _digits_count - 1) digits[_digits_count - 1] = 0;
    if (negative && i >= 0) digits[i] = 19;   // минус
    
    for (int pos = 0; pos < _digits_count; pos++) {
        _setPositionBuffer(pos, digits[pos]);
    }
}
