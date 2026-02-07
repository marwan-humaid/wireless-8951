#ifndef __LCD1602_H__
#define __LCD1602_H__

/*
 * LCD1602 driver for STC89C52RC (Keil C51)
 *
 * Pin mapping (directly wired on dev board):
 *   P0.0-P0.7 = D0-D7 (8-bit data bus)
 *   P2.7 = EN
 *   P2.6 = RS
 *   P2.5 = RW
 *
 * RS/RW/EN are declared via sbit in main.c before including this header.
 * Data bus uses P0 directly.
 */

#define LCD_DATA P0

/* HD44780 commands */
#define LCD_CMD_CLEAR       0x01
#define LCD_CMD_HOME        0x02
#define LCD_CMD_ENTRY_MODE  0x06  /* increment, no shift */
#define LCD_CMD_DISPLAY_ON  0x0C  /* display on, cursor off, blink off */
#define LCD_CMD_DISPLAY_OFF 0x08
#define LCD_CMD_FUNC_SET    0x38  /* 8-bit, 2 lines, 5x8 font */
#define LCD_CMD_LINE1       0x80  /* DDRAM address for line 1, col 0 */
#define LCD_CMD_LINE2       0xC0  /* DDRAM address for line 2, col 0 */

void _lcd_delay_us(unsigned int us);
void _lcd_delay_ms(unsigned int ms);
void lcd_command(unsigned char cmd);
void lcd_write_char(unsigned char c);
void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(unsigned char row, unsigned char col);
void lcd_write_string(char *s);

void _lcd_delay_us(unsigned int us) {
    /* ~1us per iteration at 11.0592 MHz with Keil C51 */
    while (us--) ;
}

void _lcd_delay_ms(unsigned int ms) {
    unsigned int i;
    while (ms--) {
        for (i = 0; i < 120; i++) ;
    }
}

void lcd_command(unsigned char cmd) {
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_DATA = cmd;
    LCD_EN = 1;
    _lcd_delay_us(5);
    LCD_EN = 0;
    _lcd_delay_us(50);
}

void lcd_write_char(unsigned char c) {
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_DATA = c;
    LCD_EN = 1;
    _lcd_delay_us(5);
    LCD_EN = 0;
    _lcd_delay_us(50);
}

void lcd_init(void) {
    _lcd_delay_ms(20);         /* wait >15ms after power on */

    lcd_command(LCD_CMD_FUNC_SET);
    _lcd_delay_ms(5);          /* wait >4.1ms */

    lcd_command(LCD_CMD_FUNC_SET);
    _lcd_delay_us(200);        /* wait >100us */

    lcd_command(LCD_CMD_FUNC_SET);
    _lcd_delay_us(200);

    lcd_command(LCD_CMD_DISPLAY_ON);
    _lcd_delay_us(200);

    lcd_command(LCD_CMD_CLEAR);
    _lcd_delay_ms(2);          /* clear needs >1.52ms */

    lcd_command(LCD_CMD_ENTRY_MODE);
    _lcd_delay_us(200);
}

void lcd_clear(void) {
    lcd_command(LCD_CMD_CLEAR);
    _lcd_delay_ms(2);
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
    unsigned char addr;
    if (row == 0)
        addr = LCD_CMD_LINE1 + col;
    else
        addr = LCD_CMD_LINE2 + col;
    lcd_command(addr);
}

void lcd_write_string(char *s) {
    while (*s) {
        lcd_write_char(*s++);
    }
}

#endif
