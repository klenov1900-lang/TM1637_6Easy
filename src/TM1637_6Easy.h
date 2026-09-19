#ifndef TM1637_6EASY_H
#define TM1637_6EASY_H

#include <Arduino.h>
#include <avr/pgmspace.h>
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
// ФАТАЛЬНЫЕ ОШИБКИ
// ============================================================
// При фатальной ошибке дисплей не работает, а встроенный
// светодиод (PB5, pin 13 на Nano/Uno) мигает ~3 Гц.
// Причины:
//   - clk_pin == dio_pin
//   - пин вне диапазона 2..19
//   - использован A6 или A7 (только аналоговый вход)
//   - не удалось назначить порты (внутренняя)
// Плата не выходит из этого состояния до перезагрузки.
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
    // ВЫВОД ЧИСЕЛ
    // ============================================================
    // showNumber НЕ вызывает update()!
    // Это позволяет вручную управлять точками через setDotOn/setDotOff,
    // а затем отправить всё одной командой display.update().
    //
    // Диапазоны:
    //   4 разряда:  -999 .. 9999
    //   6 разрядов: -99999 .. 999999
    // Вне диапазона — выводит "Err".
    void showNumber(int32_t number);
    
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
};

#endif
