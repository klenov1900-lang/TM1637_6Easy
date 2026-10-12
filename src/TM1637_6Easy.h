#ifndef TM1637_6EASY_H
#define TM1637_6EASY_H

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <math.h>
#include <string.h>

#define DISPLAY_BUFFER_SIZE 6
#define SEGMENTS_TABLE_SIZE 40

static const uint8_t SEGMENTS[SEGMENTS_TABLE_SIZE] PROGMEM = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, // 0-9
    0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,                         // A-F
    0x40, 0x63, 0x50,                                           // -,градус,r
    0x40,                                                       // 19: Минус
    0x00,                                                       // 20: Пробел
    0x76, 0x54, 0x78,                                           // 21-23: H,n,t
    0x5C, 0x73, 0x6D,                                           // 24-26: u,P,S
    0x38, 0x0E, 0x6E, 0x5B, 0x67, 0x06, 0x48, 0x3F              // 27-34: L,J,Y,Z,q,I,=,O
};

// ============================================================
// КОДЫ ОШИБОК
// Отображаются на дисплее как Err<код>.
// Например, код 1 → "Err1", код 2 → "Err2".
// ============================================================
#define TM1637_ERR_WRONG_MODE_3VAL   1   // showValues(v1,v2,v3) в 4-разрядном режиме
#define TM1637_ERR_WRONG_MODE_2VAL   2   // showValues(v1,v2) в 6-разрядном режиме
#define TM1637_ERR_WRONG_MODE_LEFT   3   // displayLeft() в 4-разрядном режиме
#define TM1637_ERR_WRONG_MODE_RIGHT  4   // displayRight() в 4-разрядном режиме
#define TM1637_ERR_WRONG_MODE_TEMP   5   // displayTemperature() в 4-разрядном режиме
#define TM1637_ERR_WRONG_MODE_CLEARL 6   // clearLeft() в 4-разрядном режиме
#define TM1637_ERR_WRONG_MODE_CLEARR 7   // clearRight() в 4-разрядном режиме

// ============================================================
// ДОКУМЕНТАЦИЯ ДЛЯ КОНЕЧНОГО ПОЛЬЗОВАТЕЛЯ
// ============================================================
// Библиотека отображает на дисплее "Err<код>" при неправильном
// использовании функций.
//
// Err (без кода) — showNumber() получил число вне диапазона:
//   4 разряда:  -999 .. 9999
//   6 разрядов: -99999 .. 999999
//
// Err1 — showValues(v1, v2, v3) вызвана в 4-разрядном режиме
//        (begin(2)). Эта функция работает только с 6 разрядами.
//
// Err2 — showValues(v1, v2) вызвана в 6-разрядном режиме
//        (begin(0) / begin(1)). Эта функция работает только с 4 разрядами.
//
// Err3 — displayLeft() вызвана в 4-разрядном режиме.
//
// Err4 — displayRight() вызвана в 4-разрядном режиме.
//
// Err5 — displayTemperature() вызвана в 4-разрядном режиме.
//
// Err6 — clearLeft() вызвана в 4-разрядном режиме.
//
// Err7 — clearRight() вызвана в 4-разрядном режиме.
//
// ФАТАЛЬНЫЕ ОШИБКИ (мигание встроенного светодиода PB5,
// дисплей не работает). Все фатальные ошибки мигают одинаково
// (~3 Гц), различить их по паттерну нельзя. Причины:
//
//   - clk_pin == dio_pin
//   - пин вне диапазона 2..19
//   - использован A6 или A7 (только аналоговый вход)
//   - не удалось назначить порты (внутренняя)
// ============================================================

class TM1637_6Easy {
public:
    // ============================================================
    // КОНСТРУКТОР
    // ============================================================
    TM1637_6Easy(uint8_t clk_pin, uint8_t dio_pin, uint8_t brightness = 3);
    
    // ============================================================
    // ИНИЦИАЛИЗАЦИЯ
    // ============================================================
    void begin();                    // Нормальный порядок (0,1,2,3,4,5)
    void begin(uint8_t mode);        // 0 - нормальный, 1 - китайский (2,1,0,5,4,3), 2 - 4 разряда
    
    // ============================================================
    // ОСНОВНЫЕ ФУНКЦИИ
    // ============================================================
    void clear();
    void update();
    void setBrightness(uint8_t level);
    
    // ============================================================
    // ВЫВОД ЧИСЕЛ (работают в 4 и 6 разрядах)
    // ============================================================
    // showNumber НЕ вызывает update()!
    // Это позволяет вручную управлять точками через setDotOn/setDotOff,
    // а затем отправить всё одной командой display.update().
    void showNumber(int32_t number);
    
    // showFloat вызывает update() внутри; работает в 4 и 6 разрядах
    void showFloat(float number);
    
    // showValues — перегрузка:
    //   4 разряда:  showValues(v1, v2)        → v1.v2
    //   6 разрядов: showValues(v1, v2, v3)    → v1.v2.v3
    // При неверном режиме отображает Err1 (для 3 значений) или Err2 (для 2 значений).
    void showValues(uint8_t v1, uint8_t v2);
    void showValues(uint8_t v1, uint8_t v2, uint8_t v3);
    
    // ============================================================
    // ВЫВОД СТРОК И СИМВОЛОВ
    // ============================================================
    // showString поддерживает: цифры, A-F, H,n,t,u,P,S,L,J,Y,Z,q,I,=,O,
    // '-', '^', '.', ' '
    void showString(const char* str);
    void showChar(uint8_t position, uint8_t symbol);
    
    // showChars: в 4-разрядном режиме используются только s1..s4,
    // s5 и s6 игнорируются (соответствуют разрядам 5 и 6, которых нет).
    void showChars(uint8_t s1, uint8_t s2, uint8_t s3, 
                   uint8_t s4, uint8_t s5, uint8_t s6);
    
    // ============================================================
    // ТОЛЬКО ДЛЯ 6 РАЗРЯДОВ (begin(0) или begin(1))
    // В режиме begin(2) (4 разряда) — отображают Err3..Err7.
    // ============================================================
    void displayLeft(int32_t number);      // Err3 при неверном режиме
    void displayRight(int32_t number);     // Err4 при неверном режиме
    void clearLeft();                      // Err6 при неверном режиме
    void clearRight();                     // Err7 при неверном режиме
    void displayTemperature(float temperature);  // Err5 при неверном режиме
    
    // ============================================================
    // УПРАВЛЕНИЕ ТОЧКАМИ
    // ============================================================
    void setDotOn(uint8_t position);
    void setDotOff(uint8_t position);
    void clearDots();
    void setDots(uint8_t dots);
    
    // ============================================================
    // УПРАВЛЕНИЕ ПИТАНИЕМ
    // ============================================================
    void displayOff();
    void displayOn();

private:
    uint8_t _clk_pin;
    uint8_t _dio_pin;
    uint8_t _brightness;
    uint8_t _dots;
    uint8_t _digits_count;
    uint8_t _display_buffer[DISPLAY_BUFFER_SIZE];
    uint8_t _display_order[DISPLAY_BUFFER_SIZE];
    
    volatile uint8_t* _clk_port;
    volatile uint8_t* _dio_port;
    volatile uint8_t* _clk_ddr;
    volatile uint8_t* _dio_ddr;
    volatile uint8_t* _clk_pin_reg;
    volatile uint8_t* _dio_pin_reg;
    uint8_t _clk_mask;
    uint8_t _dio_mask;
    
    // ============================================================
    // ВНУТРЕННИЕ
    // ============================================================
    void _initPins();
    inline void _delay();
    void _start();
    void _stop();
    bool _writeByte(uint8_t data);
    void _sendData();
    inline void _setPinHigh(volatile uint8_t* port, uint8_t mask);
    inline void _setPinLow(volatile uint8_t* port, uint8_t mask);
    inline bool _readPin(volatile uint8_t* pin_reg, uint8_t mask);
    void _assignPin(uint8_t pin, volatile uint8_t* &port,
                    volatile uint8_t* &ddr, volatile uint8_t* &pin_reg,
                    uint8_t &mask);
    void _setOrder(uint8_t mode);
    void _errorHang();
    uint8_t _encodeChar(uint8_t symbol);
    void _setPositionBuffer(uint8_t position, uint8_t symbol);
    void _showError(uint8_t code);
    
    // Внутренние реализации для 4/6 разрядов
    void _showFloat4(float number);
    void _showFloat6(float number);
    void _showTwoValues(uint8_t val1, uint8_t val2);
    void _showThreeValues(uint8_t val1, uint8_t val2, uint8_t val3);
};

#endif
