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
// ВЫВОД КОДА ОШИБКИ
// Отображает "Err<код>" в начале дисплея.
// Работает в любом режиме (4 или 6 разрядов).
// ============================================================

void TM1637_6Easy::_showError(uint8_t code) {
    clear();
    clearDots();
    
    _setPositionBuffer(0, 14);   // 'E'
    _setPositionBuffer(1, 18);   // 'r'
    _setPositionBuffer(2, 18);   // 'r'
    
    if (code < 10) {
        _setPositionBuffer(3, code);
    } else {
        _setPositionBuffer(3, code / 10);
        _setPositionBuffer(4, code % 10);
    }
    
    update();
}

// ============================================================
// showNumber
// ВНИМАНИЕ: не вызывает update()!
// Это позволяет вручную управлять точками через setDotOn/setDotOff,
// а затем отправить всё одной командой display.update().
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
            _setPositionBuffer(pos++, 19);
        }
        _setPositionBuffer(pos++, 14);
        _setPositionBuffer(pos++, 18);
        _setPositionBuffer(pos,   18);
        return;
    }
    
    uint8_t digits[DISPLAY_BUFFER_SIZE];
    for (int i = 0; i < _digits_count; i++) digits[i] = 20;
    
    bool negative = (number < 0);
    uint32_t absVal = abs(number);
    
    int i = _digits_count - 1;
    while (absVal > 0 && i >= 0) {
        digits[i--] = absVal % 10;
        absVal /= 10;
    }
    
    if (absVal == 0 && i == _digits_count - 1) digits[_digits_count - 1] = 0;
    if (negative && i >= 0) digits[i] = 19;
    
    for (int pos = 0; pos < _digits_count; pos++) {
        _setPositionBuffer(pos, digits[pos]);
    }
}

// ============================================================
// showFloat - диспетчер 4/6 разрядов
// ============================================================

void TM1637_6Easy::showFloat(float number) {
    if (_digits_count == 4) {
        _showFloat4(number);
    } else {
        _showFloat6(number);
    }
}

// ============================================================
// showValues - перегрузка
// ============================================================

void TM1637_6Easy::showValues(uint8_t v1, uint8_t v2) {
    _showTwoValues(v1, v2);
}

void TM1637_6Easy::showValues(uint8_t v1, uint8_t v2, uint8_t v3) {
    _showThreeValues(v1, v2, v3);
}

// ============================================================
// _showFloat4
// ============================================================

void TM1637_6Easy::_showFloat4(float number) {
    clear();
    clearDots();
    
    bool is_negative = (number < 0);
    float abs_val = fabs(number);
    
    if (abs_val < 0.0000001f) {
        _setPositionBuffer(1, 0);
        _setPositionBuffer(2, 0);
        _setPositionBuffer(3, 0);
        setDotOn(1);
        update();
        return;
    }
    
    long n_scaled;
    int dot_pos = -1;
    float temp_val = abs_val;
    
    if (is_negative) {
        if (temp_val < 9.995f) {
            n_scaled = lroundf(temp_val * 100.0f);
            dot_pos = 1;
        } else if (temp_val < 99.995f) {
            n_scaled = lroundf(temp_val * 10.0f);
            dot_pos = 2;
        } else if (temp_val < 999.5f) {
            n_scaled = lroundf(temp_val);
            dot_pos = -1;
        } else {
            showChars(19, 14, 18, 18, 20, 20);
            return;
        }
    } else {
        if (temp_val < 99.995f) {
            n_scaled = lroundf(temp_val * 100.0f);
            dot_pos = 1;
        } else if (temp_val < 999.995f) {
            n_scaled = lroundf(temp_val * 10.0f);
            dot_pos = 2;
        } else if (temp_val < 9999.5f) {
            n_scaled = lroundf(temp_val);
            dot_pos = -1;
        } else {
            showChars(14, 18, 18, 20, 20, 20);
            return;
        }
    }
    
    if (is_negative) n_scaled = -n_scaled;
    
    uint8_t data[4] = {20, 20, 20, 20};
    int pos = 3;
    long n_abs = labs(n_scaled);
    
    if (dot_pos == 1) {
        data[pos--] = n_abs % 10; n_abs /= 10;
        data[pos--] = n_abs % 10; n_abs /= 10;
    } else if (dot_pos == 2) {
        data[pos--] = n_abs % 10; n_abs /= 10;
    }
    
    data[pos--] = n_abs % 10; n_abs /= 10;
    while (n_abs > 0 && pos >= 0) {
        data[pos--] = n_abs % 10;
        n_abs /= 10;
    }
    
    if (is_negative && pos >= 0) data[pos] = 19;
    
    if (dot_pos != -1) setDotOn(dot_pos);
    
    for (int i = 0; i < 4; i++) {
        _setPositionBuffer(i, data[i]);
    }
    update();
}

// ============================================================
// _showFloat6
// ============================================================

void TM1637_6Easy::_showFloat6(float number) {
    clear();
    clearDots();
    
    bool is_negative = (number < 0);
    float abs_val = fabs(number);
    
    if (abs_val < 0.0000001f) {
        uint8_t data[6] = {20, 20, 20, 0, 0, 0};
        setDotOn(3);
        for (int i = 0; i < 6; i++) {
            _setPositionBuffer(i, data[i]);
        }
        update();
        return;
    }
    
    long n_scaled;
    int dot_pos = -1;
    float temp_val = abs_val;
    
    if (is_negative) {
        if (temp_val < 99.995f) {
            n_scaled = lroundf(temp_val * 100.0f);
            dot_pos = 3;
        } else if (temp_val < 999.995f) {
            n_scaled = lroundf(temp_val * 10.0f);
            dot_pos = 4;
        } else if (temp_val < 99999.5f) {
            n_scaled = lroundf(temp_val);
            dot_pos = -1;
        } else {
            showChars(19, 14, 18, 18, 20, 20);
            return;
        }
    } else {
        if (temp_val < 999.995f) {
            n_scaled = lroundf(temp_val * 100.0f);
            dot_pos = 3;
        } else if (temp_val < 9999.995f) {
            n_scaled = lroundf(temp_val * 10.0f);
            dot_pos = 4;
        } else if (temp_val < 999999.5f) {
            n_scaled = lroundf(temp_val);
            dot_pos = -1;
        } else {
            showChars(14, 18, 18, 20, 20, 20);
            return;
        }
    }
    
    if (is_negative) n_scaled = -n_scaled;
    
    uint8_t data[6] = {20, 20, 20, 20, 20, 20};
    int pos = 5;
    long n_abs = labs(n_scaled);
    
    if (dot_pos == 3) {
        data[pos--] = n_abs % 10; n_abs /= 10;
        data[pos--] = n_abs % 10; n_abs /= 10;
    } else if (dot_pos == 4) {
        data[pos--] = n_abs % 10; n_abs /= 10;
    }
    
    data[pos--] = n_abs % 10; n_abs /= 10;
    while (n_abs > 0 && pos >= 0) {
        data[pos--] = n_abs % 10;
        n_abs /= 10;
    }
    
    if (is_negative && pos >= 0) data[pos] = 19;
    
    if (dot_pos != -1) setDotOn(dot_pos);
    
    for (int i = 0; i < 6; i++) {
        _setPositionBuffer(i, data[i]);
    }
    update();
}

// ============================================================
// _showTwoValues (4 разряда)
// ============================================================

void TM1637_6Easy::_showTwoValues(uint8_t val1, uint8_t val2) {
    if (_digits_count != 4) {
        _showError(TM1637_ERR_WRONG_MODE_2VAL);
        return;
    }
    
    if (val1 > 99) val1 = 99;
    if (val2 > 99) val2 = 99;
    
    clear();
    clearDots();
    setDotOn(1);
    
    _setPositionBuffer(0, val1 / 10);
    _setPositionBuffer(1, val1 % 10);
    _setPositionBuffer(2, val2 / 10);
    _setPositionBuffer(3, val2 % 10);
    
    update();
}

// ============================================================
// _showThreeValues (6 разрядов)
// ============================================================

void TM1637_6Easy::_showThreeValues(uint8_t val1, uint8_t val2, uint8_t val3) {
    if (_digits_count != 6) {
        _showError(TM1637_ERR_WRONG_MODE_3VAL);
        return;
    }
    
    if (val1 > 99) val1 = 99;
    if (val2 > 99) val2 = 99;
    if (val3 > 99) val3 = 99;
    
    clear();
    clearDots();
    setDotOn(1);
    setDotOn(3);
    
    _setPositionBuffer(0, val1 / 10);
    _setPositionBuffer(1, val1 % 10);
    _setPositionBuffer(2, val2 / 10);
    _setPositionBuffer(3, val2 % 10);
    _setPositionBuffer(4, val3 / 10);
    _setPositionBuffer(5, val3 % 10);
    
    update();
}

// ============================================================
// showChar
// ============================================================

void TM1637_6Easy::showChar(uint8_t position, uint8_t symbol) {
    if (position >= _digits_count) return;
    _setPositionBuffer(position, symbol);
    update();
}

// ============================================================
// showChars
// В 4-разрядном режиме s5 и s6 игнорируются.
// ============================================================

void TM1637_6Easy::showChars(uint8_t s1, uint8_t s2, uint8_t s3, 
                              uint8_t s4, uint8_t s5, uint8_t s6) {
    clear();
    clearDots();
    
    uint8_t chars[6] = {s1, s2, s3, s4, s5, s6};
    for (uint8_t i = 0; i < _digits_count; i++) {
        _setPositionBuffer(i, chars[i]);
    }
    update();
}

// ============================================================
// showString
// Поддерживает: цифры, A-F, H,n,t,u,P,S,L,J,Y,Z,q,I,=,O, '-', '^', '.', ' '
// '.' — ставит точку в предыдущем разряде
// ============================================================

void TM1637_6Easy::showString(const char* str) {
    if (str == nullptr) return;
    
    clear();
    clearDots();
    
    uint8_t pos = 0;
    uint8_t i = 0;
    
    while (str[i] != '\0' && pos < _digits_count) {
        char c = str[i];
        
        if (c == '.') {
            if (pos > 0) setDotOn(pos - 1);
            i++;
            continue;
        }
        
        uint8_t idx = 20;
        
        if (c >= '0' && c <= '9') {
            idx = c - '0';
        } else {
            switch(c) {
                case 'A': case 'a': idx = 10; break;
                case 'B': case 'b': idx = 11; break;
                case 'C': case 'c': idx = 12; break;
                case 'D': case 'd': idx = 13; break;
                case 'E': case 'e': idx = 14; break;
                case 'F': case 'f': idx = 15; break;
                case '-': idx = 16; break;
                case '^': idx = 17; break;
                case '=': idx = 33; break;
                case 'H': case 'h': idx = 21; break;
                case 'I': case 'i': idx = 32; break;
                case 'J': case 'j': idx = 28; break;
                case 'L': case 'l': idx = 27; break;
                case 'N': case 'n': idx = 22; break;
                case 'O': case 'o': idx = 34; break;
                case 'P': case 'p': idx = 25; break;
                case 'Q': case 'q': idx = 31; break;
                case 'R': case 'r': idx = 18; break;
                case 'S': case 's': idx = 26; break;
                case 'T': case 't': idx = 23; break;
                case 'U': case 'u': idx = 24; break;
                case 'Y': case 'y': idx = 29; break;
                case 'Z': case 'z': idx = 30; break;
                case ' ': idx = 20; break;
                default: idx = 20; break;
            }
        }
        
        _setPositionBuffer(pos, idx);
        pos++;
        i++;
    }
    
    update();
}

// ============================================================
// displayLeft - только 6 разрядов
// ============================================================

void TM1637_6Easy::displayLeft(int32_t number) {
    if (_digits_count != 6) {
        _showError(TM1637_ERR_WRONG_MODE_LEFT);
        return;
    }
    
    if (number < -999) number = -999;
    if (number > 999) number = 999;
    
    uint8_t digits[6];
    for (int i = 0; i < 6; i++) digits[i] = 20;
    
    bool negative = (number < 0);
    uint32_t absVal = abs(number);
    
    int pos = 2;
    while (absVal > 0 && pos >= 0) {
        digits[pos--] = absVal % 10;
        absVal /= 10;
    }
    
    if (negative && pos >= 0) digits[pos] = 19;
    
    clear();
    clearDots();
    for (int i = 0; i < 3; i++) {
        if (digits[i] < SEGMENTS_TABLE_SIZE) {
            _display_buffer[i] = pgm_read_byte(&SEGMENTS[digits[i]]);
        }
    }
    update();
}

// ============================================================
// displayRight - только 6 разрядов
// ============================================================

void TM1637_6Easy::displayRight(int32_t number) {
    if (_digits_count != 6) {
        _showError(TM1637_ERR_WRONG_MODE_RIGHT);
        return;
    }
    
    if (number < -999) number = -999;
    if (number > 999) number = 999;
    
    uint8_t digits[6];
    for (int i = 0; i < 6; i++) digits[i] = 20;
    
    bool negative = (number < 0);
    uint32_t absVal = abs(number);
    
    int pos = 5;
    while (absVal > 0 && pos >= 3) {
        digits[pos--] = absVal % 10;
        absVal /= 10;
    }
    
    if (negative && pos >= 3) digits[pos] = 19;
    
    clear();
    clearDots();
    for (int i = 3; i < 6; i++) {
        if (digits[i] < SEGMENTS_TABLE_SIZE) {
            _display_buffer[i] = pgm_read_byte(&SEGMENTS[digits[i]]);
        }
    }
    update();
}

// ============================================================
// clearLeft - только 6 разрядов
// ============================================================

void TM1637_6Easy::clearLeft() {
    if (_digits_count != 6) {
        _showError(TM1637_ERR_WRONG_MODE_CLEARL);
        return;
    }
    
    clearDots();
    for (int i = 0; i < 3; i++) {
        _display_buffer[i] = 0x00;
    }
    update();
}

// ============================================================
// clearRight - только 6 разрядов
// ============================================================

void TM1637_6Easy::clearRight() {
    if (_digits_count != 6) {
        _showError(TM1637_ERR_WRONG_MODE_CLEARR);
        return;
    }
    
    clearDots();
    for (int i = 3; i < 6; i++) {
        _display_buffer[i] = 0x00;
    }
    update();
}

// ============================================================
// displayTemperature - только 6 разрядов
// При отрицательной температуре точка в позиции 3 (явно).
// ============================================================

void TM1637_6Easy::displayTemperature(float temperature) {
    if (_digits_count != 6) {
        _showError(TM1637_ERR_WRONG_MODE_TEMP);
        return;
    }

    const uint8_t BLANK = 20, MINUS = 19, DEG = 17, C = 12;

    if (temperature < -999.0f) temperature = -999.0f;
    if (temperature > 9999.0f) temperature = 9999.0f;   // было 999.9

    uint8_t digits[6] = {BLANK, BLANK, BLANK, BLANK, DEG, C};
    int dot_pos = -1;

    bool  negative = (temperature < 0);
    float abs_val  = fabsf(temperature);

    if (negative && abs_val >= 100.0f) {
        // ---- МИНУС, 100..999, целые (как было) -----------------------
        int whole = (int)(abs_val + 0.5f);
        if (whole > 999) whole = 999;

        digits[0] = MINUS;
        digits[1] = (whole / 100) % 10;
        digits[2] = (whole / 10)  % 10;
        digits[3] =  whole        % 10;
        // точка не горит
    } else if (!negative && abs_val >= 1000.0f) {
        // ---- ПЛЮС, 1000..9999, целые --------------------------------
        int whole = (int)(abs_val + 0.5f);
        if (whole > 9999) whole = 9999;

        digits[0] = (whole / 1000) % 10;
        digits[1] = (whole / 100)  % 10;
        digits[2] = (whole / 10)   % 10;
        digits[3] =  whole         % 10;
        // точка не горит
    } else {
        // ---- всё остальное: десятые есть -----------------------------
        int scaled = (int)(abs_val * 10.0f + 0.5f);
        uint8_t hundreds = (scaled / 1000) % 10;
        uint8_t tens     = (scaled / 100)  % 10;
        uint8_t ones     = (scaled / 10)   % 10;
        uint8_t frac     =  scaled         % 10;

        if (abs_val >= 100.0f) {
            // XXX.X  (только плюс, 100.0..999.9)
            digits[0] = hundreds;
            digits[1] = tens;
            digits[2] = ones;
            digits[3] = frac;
            dot_pos   = 2;
        } else if (abs_val >= 10.0f) {
            // _XX.X  или -XX.X
            digits[1] = tens;
            digits[2] = ones;
            digits[3] = frac;
            dot_pos   = 2;
            if (negative) digits[0] = MINUS;
        } else {
            // __X.X  или __-X.X
            digits[2] = ones;
            digits[3] = frac;
            dot_pos   = 2;
            if (negative) digits[1] = MINUS;
        }
    }

    clear();
    clearDots();
    if (dot_pos >= 0) setDotOn(dot_pos);

    for (int i = 0; i < 6; i++) {
        if (digits[i] != BLANK) {
            _setPositionBuffer(i, digits[i]);
        }
    }
    update();
}
