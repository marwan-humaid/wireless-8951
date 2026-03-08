#include <reg52.h>

/* LCD1602 control pins */
sbit LCD_EN = P2^7;
sbit LCD_RS = P2^6;
sbit LCD_RW = P2^5;

/* NRF24L01 SPI pins */
sbit CE   = P1^2;
sbit CSN  = P1^3;
sbit SCK  = P1^7;
sbit MOSI = P1^1;
sbit MISO = P1^6;

#include "lcd1602.h"
#include "nrf24l01.h"

char xdata rx_buf[32];
char addr[] = {0x30, 0x30, 0x30, 0x30, 0x31};

unsigned char cursor_row;
unsigned char cursor_col;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 200; j++) ;
}

void cursor_advance(void) {
    cursor_col++;
    if (cursor_col >= 16) {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= 2) {
            cursor_row = 0;
        }
        lcd_set_cursor(cursor_row, cursor_col);
    }
}

void process_char(unsigned char ch) {
    if (ch == 0x08) {
        /* backspace */
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = 15;
        }
        lcd_set_cursor(cursor_row, cursor_col);
        lcd_write_char(' ');
        lcd_set_cursor(cursor_row, cursor_col);
    } else if (ch == 0x0D) {
        /* enter: line 2 or clear */
        if (cursor_row == 0) {
            cursor_row = 1;
            cursor_col = 0;
            lcd_set_cursor(1, 0);
            lcd_write_string("                ");
            lcd_set_cursor(1, 0);
        } else {
            lcd_clear();
            cursor_row = 0;
            cursor_col = 0;
        }
    } else if (ch == 0x0C) {
        /* Ctrl+L: clear */
        lcd_clear();
        cursor_row = 0;
        cursor_col = 0;
    } else if (ch >= 0x20 && ch <= 0x7E) {
        /* printable ASCII */
        lcd_write_char(ch);
        cursor_advance();
    }
}

void main(void) {
    unsigned char i, count;
    bool ok;

    lcd_init();
    delay_ms(100);

    lcd_set_cursor(0, 0);
    lcd_write_string("NRF24L01  Init..");
    delay_ms(100);

    if (!nrf_init()) {
        lcd_set_cursor(0, 0);
        lcd_write_string("NRF Init FAILED!");
        lcd_set_cursor(1, 0);
        lcd_write_string("Check wiring    ");
        while (1) ;
    }

    lcd_set_cursor(0, 0);
    lcd_write_string("Keyboard RX OK  ");
    lcd_set_cursor(1, 0);
    lcd_write_string("CH:108  1Mbps   ");
    delay_ms(1500);

    lcd_clear();
    cursor_row = 0;
    cursor_col = 0;

    while (1) {
        ok = nrf_recv(addr, rx_buf, 500);
        if (ok) {
            count = (unsigned char)rx_buf[0];
            if (count > 31) count = 31;
            for (i = 0; i < count; i++) {
                process_char((unsigned char)rx_buf[i + 1]);
            }
        }
    }
}
