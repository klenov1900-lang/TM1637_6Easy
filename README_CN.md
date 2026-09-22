# TM1637_6Easy

![Version](https://img.shields.io/badge/version-2.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Arduino](https://img.shields.io/badge/Arduino-Nano%20%7C%20Uno%20%7C%20Pro%20Mini-teal)
![Platform](https://img.shields.io/badge/platform-AVR-orange)
![Status](https://img.shields.io/badge/status-stable-brightgreen)

**语言：** [Русский](README.md) · [中文](README_CN.md)

基于 TM1637 的高性能 4 位和 6 位 LED 显示屏库，适用于 ATmega328P（Arduino Uno、Nano、Pro Mini）板。

TM1637 协议时序由 `nop` 时钟延迟而非 `delayMicroseconds()` 形成，引脚控制直接通过寄存器 `PORTx` / `DDRx` / `PINx`。因此该库在带有**陶瓷谐振器**的廉价克隆版上也能稳定运行，而使用 `digitalWrite()` 的解决方案可能会出现故障。

## ✨ 特点

- ✅ **直接寄存器操作**（`PORTx` / `DDRx` / `PINx`）— 无需 `digitalWrite`
- ✅ **协议延迟为单个 `nop`** — 不依赖 `F_CPU`
- ✅ **引脚表存储在 PROGMEM** — 节省 RAM
- ✅ **支持 4 和 6 位**
- ✅ **6 位模块的两种位序**（正常 / 非标准）
- ✅ **支持任意引脚**：D2–D13, A0–A5
- ✅ **最小开销** — 最大协议速度
- ✅ **致命错误** — PB5 内置 LED 闪烁
- ✅ **可在 Arduino `init()` 之前工作**（致命错误时闪烁）

## ⚙️ 要求与兼容性

- **微控制器：** ATmega328P（8 位 AVR）。
- **开发板：** 基于 ATmega328P 的 Arduino Uno、Nano、Pro Mini。其他 AVR（如 Mega）需要修改 `PIN_MAP` 表。
- **时钟频率：** 时序针对 **16 MHz**（5V 板标准）设计。在 8 MHz 下延迟加倍——可以工作，但需要验证。
- **其他平台**（ESP8266、ESP32、STM32）：**不支持** —— 没有对 AVR 寄存器的直接访问。

## 📦 安装

### 方法 1：通过 Arduino IDE 库管理器（推荐）

1. 打开 Arduino IDE
2. 转到 **Sketch** → **Include Library** → **Manage Libraries...**
3. 在搜索框中输入 `TM1637_6Easy`
4. 点击 **Install**

### 方法 2：手动（ZIP）

1. 从 [最新版本](https://github.com/klenov1900-lang/TM1637_6Easy/releases) 下载 ZIP 归档
2. 在 Arduino IDE 中：**Sketch** → **Include Library** → **Add .ZIP Library...**
3. 选择下载的归档文件

### 方法 3：通过 Git

```bash
cd ~/Arduino/libraries/
git clone https://github.com/klenov1900-lang/TM1637_6Easy.git
```

## 🔌 连接

### TM1637 → Arduino Nano

| TM1637 | Arduino |
|--------|---------|
| VCC | +5V |
| GND | GND |
| CLK | D2–D13, A0–A5 |
| DIO | D2–D13, A0–A5 |

### 📊 可用引脚

| 引脚 | 状态 | 备注 |
|------|------|------|
| D2–D7 | ✅ 可用 | 数字引脚 PORTD |
| D8–D13 | ✅ 可用 | 数字引脚 PORTB |
| A0–A5 | ✅ 可用 | 模拟引脚作为数字（PORTC） |
| D0, D1 | ❌ 不可用 | UART 占用 |
| A6, A7 | ❌ 不可用 | 仅模拟输入 |

### 🎯 推荐引脚

```cpp
TM1637_6Easy display(8, 9, 3);    // CLK=D8, DIO=D9
TM1637_6Easy display(4, 5, 3);    // CLK=D4, DIO=D5
TM1637_6Easy display(A0, A1, 3);  // CLK=A0, DIO=A1
```

## 🚀 快速入门

### 6 位，非标准顺序

![六位数码管接线图](docs/image1.png)

```cpp
#include "TM1637_6Easy.h"

TM1637_6Easy display(8, 9, 0);            // CLK=8, DIO=9, brightness=0

void setup() {
    display.begin(1);                     // 6 位，非标准顺序
    display.showNumber(123456);
    display.setDotOn(2);                  // 在第 2 位点亮小数点
                                          // 显示效果：[1][2][3.][4][5][6]
    display.update();
    delay(2000);
}

void loop() {}
```

### 4 位

![四位数码管接线图](docs/image2.png)

```cpp
#include "TM1637_6Easy.h"

TM1637_6Easy display(7, 6, 0);            // CLK=7, DIO=6, brightness=0

void setup() {
    display.begin(2);                     // 4 位
    display.showNumber(1234);
    display.setDotOn(2);                  // 在第 2 位点亮小数点
                                          // 显示效果：[1][2][3.][4]
    display.update();
    delay(2000);
}

void loop() {}
```

### 同时使用两个显示器（4 和 6 位）

![两个显示器接线图](docs/image.png)

```cpp
#include "TM1637_6Easy.h"

TM1637_6Easy display6(8, 9, 5);           // CLK=8, DIO=9, brightness=5
TM1637_6Easy display4(7, 6, 3);           // CLK=7, DIO=6, brightness=3

void setup() {
    display6.begin(0);                    // 6 位，正常顺序
    display6.showNumber(1234);
    display6.update();

    display4.begin(2);                    // 4 位
    display4.showNumber(1234);
    display4.update();
}

void loop() {}
```

## 🎛️ 模式

```cpp
display.begin(0);  // 6 位，正常顺序 (0,1,2,3,4,5)
display.begin(1);  // 6 位，非标准顺序 (2,1,0,5,4,3)
display.begin(2);  // 4 位
```

- 如果您是**自制模块** —— 通常是 `begin(0)`
- 如果是**来自 AliExpress 的模块** —— 通常是 `begin(1)`
- 如果显示器上的数字**顺序不对** —— 尝试另一种模式

## 📚 API

### 初始化

| 函数 | 位数 | 描述 |
|------|------|------|
| `begin()` | 6 | 等同于 `begin(0)` |
| `begin(mode)` | 4/6 | `0`、`1` — 6 位；`2` — 4 位 |

### 基础功能

| 函数 | 位数 | 描述 |
|------|------|------|
| `clear()` | 4/6 | 清除缓冲区（不调用 `update()`） |
| `update()` | 4/6 | 将缓冲区发送到显示器 |
| `setBrightness(0..7)` | 4/6 | 亮度 |

### 显示数字

| 函数 | 位数 | 描述 |
|------|------|------|
| `showNumber(n)` | 4/6 | **不调用** `update()`。需要 `display.update()`。范围：4 位 `-999..9999`，6 位 `-99999..999999`。超出范围则显示 `Err` 或 `-Err`。 |

### 小数点控制

| 函数 | 位数 | 描述 |
|------|------|------|
| `setDotOn(pos)` | 4/6 | 打开小数点 |
| `setDotOff(pos)` | 4/6 | 关闭小数点 |
| `clearDots()` | 4/6 | 关闭所有小数点 |
| `setDots(mask)` | 4/6 | 按掩码设置 |

> **重要：** 小数点仅在 `update()` 时生效。如果在 `showNumber()`（它不调用 `update()`）之后更改了小数点，需要手动调用 `update()`。

### 电源

| 函数 | 位数 | 描述 |
|------|------|------|
| `displayOff()` | 4/6 | 关闭显示器 |
| `displayOn()` | 4/6 | 打开显示器 |

## ⚡ `showNumber` 和小数点

**关键区别：** `showNumber(n)` **不调用** `update()` —— 这使得在发送前手动控制小数点成为可能：

```cpp
display.showNumber(1234);
display.setDotOn(1);
display.update();       // ← 没有这一步小数点不会出现！
```

调用顺序：

1. `showNumber(n)` —— 填充数字缓冲区
2. `setDotOn(pos)` / `setDotOff(pos)` —— 设置小数点
3. `update()` —— 发送到显示器

`setDotOn` 中的位置号是从左到右计数的指示器位置，从零开始：

```
setDotOn(0) → [1.][2][3][4][5][6]
setDotOn(1) → [1][2.][3][4][5][6]
setDotOn(2) → [1][2][3.][4][5][6]
setDotOn(3) → [1][2][3][4.][5][6]
setDotOn(4) → [1][2][3][4][5.][6]
setDotOn(5) → [1][2][3][4][5][6.]
```

## 📊 使用示例

### 显示带小数点的数字（时钟）

```cpp
display.showNumber(1234);
display.setDots(0b00000010);  // 小数点位于 [1][2.][3][4]
display.update();
```

### 闪烁小数点

```cpp
display.showNumber(1234);
if (dot_state) {
    display.setDots(0b00001000);  // 小数点位于 [1][2][3][4.]
} else {
    display.clearDots();
}
display.update();
```

### 秒计数器

```cpp
void loop() {
    static uint32_t last = 0;
    if (millis() - last >= 1000) {
        last = millis();
        display.showNumber((millis() / 1000) % 100);
        display.update();
    }
}
```

## ⚠️ 容错

库**忽略来自显示器的确认失败（NACK）**。如果模块损坏，程序不会卡死，而只是将数据发送到"空气中"。当前值保存在缓冲区中，在更换模块后会被显示。

### 致命错误

发生致命错误时，显示器不工作，而内置 LED（**PB5，Nano/Uno 上为 pin 13**）以约 **3 Hz** 的频率闪烁。

**原因：**

- `clk_pin == dio_pin`
- 引脚超出 `2..19` 范围
- 使用了 `A6` 或 `A7`（仅模拟输入）
- 无法分配端口（内部错误）

板子**在重启前不会退出此状态**。

## 📝 更新日志

| 版本 | 日期 | 更改 |
|------|------|------|
| **v2.0.0** | 2026-09-19 | 完全重构 API。添加 4 位显示器支持。更新 README，添加示例。 |
| v1.0.2 | 2026-09-05 | 版本号更新至 1.0.2 |

## 📄 许可证

MIT License。参见 [LICENSE](LICENSE)。

## 📧 联系方式

- **Email:** gray_wolf19@mail.ru
- **GitHub:** [klenov1900-lang](https://github.com/klenov1900-lang)

---

⭐ 如果本库对您有帮助，请在 GitHub 上给个星标！⭐
