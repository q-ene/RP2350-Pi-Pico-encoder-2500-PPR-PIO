#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "lcd_i2c.h"

#define LCD_ADDR   0x27
#define LCD_SDA    4
#define LCD_SCL    5
#define I2C_FREQ   100000

// PCF8574: бит0=RS, бит1=RW, бит2=E, бит3=подсветка

static void i2c_write_byte(uint8_t b) {
    i2c_write_blocking(i2c0, LCD_ADDR, &b, 1, false);
}

static void lcd_write4(uint8_t nib, uint8_t rs) {
    uint8_t data = (uint8_t)((nib & 0xF0) | rs | 0x08);
    i2c_write_byte((uint8_t)(data | 0x04));    // E = 1
    i2c_write_byte((uint8_t)(data & ~0x04));   // E = 0
}

static void lcd_send(uint8_t val, uint8_t rs) {
    lcd_write4((uint8_t)(val & 0xF0), rs);
    lcd_write4((uint8_t)(val << 4), rs);
}

static void lcd_cmd(uint8_t c) {
    lcd_send(c, 0);
}

static void lcd_data(uint8_t d) {
    lcd_send(d, 1);
}

void lcd_init(void) {
    i2c_init(i2c0, I2C_FREQ);
    gpio_set_function(LCD_SDA, GPIO_FUNC_I2C);
    gpio_set_function(LCD_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_SDA);
    gpio_pull_up(LCD_SCL);

    sleep_ms(50);
    lcd_write4(0x30, 0); sleep_ms(5);
    lcd_write4(0x30, 0); sleep_ms(1);
    lcd_write4(0x30, 0); sleep_ms(1);
    lcd_write4(0x20, 0);          // 4-битный режим
    lcd_cmd(0x28);                // 4 бита, 2 строки, 5x8
    lcd_cmd(0x0C);                // дисплей вкл, курсор выкл
    lcd_cmd(0x06);                // инкремент адреса
    lcd_cmd(0x01);                // очистка
    sleep_ms(2);
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    static const uint8_t row_off[2] = {0x00, 0x40};
    lcd_cmd((uint8_t)(0x80 | row_off[row & 1] | col));
}

void lcd_string(const char *s) {
    while (*s) lcd_data((uint8_t)*s++);
}

void lcd_clear(void) {
    lcd_cmd(0x01);
    sleep_ms(2);
}
