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

char xdata tx_buf[32];
char addr[] = {0x30, 0x30, 0x30, 0x30, 0x31};  /* "00001" */
unsigned int tx_count;
unsigned int ok_count;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 200; j++) ;
}

/* Write a hex byte like "0E" to LCD at current cursor */
void lcd_write_hex(unsigned char val) {
    unsigned char hi = val >> 4;
    unsigned char lo = val & 0x0F;
    lcd_write_char(hi < 10 ? '0' + hi : 'A' + hi - 10);
    lcd_write_char(lo < 10 ? '0' + lo : 'A' + lo - 10);
}

/* Write a 4-digit decimal number to LCD at current cursor */
void lcd_write_dec4(unsigned int val) {
    lcd_write_char('0' + (val / 1000) % 10);
    lcd_write_char('0' + (val / 100) % 10);
    lcd_write_char('0' + (val / 10) % 10);
    lcd_write_char('0' + val % 10);
}

void main(void) {
    unsigned char status;
    bool ok;

    lcd_init();
    delay_ms(100);

    /* Show init screen */
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

    /* Show init success + config */
    status = _nrf_get_reg(STATUS);
    lcd_set_cursor(0, 0);
    lcd_write_string("NRF OK  ST:0x");
    lcd_write_hex(status);

    lcd_set_cursor(1, 0);
    lcd_write_string("CH:108  1Mbps   ");
    delay_ms(1500);

    lcd_set_cursor(0, 0);
    lcd_write_string("CH:");
    lcd_write_hex(_nrf_get_reg(RF_CH));
    lcd_write_string(" RF:");
    lcd_write_hex(_nrf_get_reg(RF_SETUP));
    lcd_set_cursor(1, 0);
    lcd_write_string("CF:");
    lcd_write_hex(_nrf_get_reg(CONFIG));
    lcd_write_string(" AA:");
    lcd_write_hex(_nrf_get_reg(EN_AA));
    delay_ms(2000);

    /* TX loop */
    tx_count = 0;
    ok_count = 0;

    while (1) {
        /* Fill payload: counter in first 2 bytes, 0xAA pattern in rest */
        tx_buf[0] = (unsigned char)(tx_count >> 8);
        tx_buf[1] = (unsigned char)(tx_count & 0xFF);
        tx_buf[2] = 0xAA;
        tx_buf[3] = 0x55;

        ok = nrf_send(addr, tx_buf);
        tx_count++;
        if (ok) ok_count++;

        /* Line 1: TX count and OK count */
        lcd_set_cursor(0, 0);
        lcd_write_string("TX:");
        lcd_write_dec4(tx_count);
        lcd_write_string(" OK:");
        lcd_write_dec4(ok_count);

        /* Line 2: last status + result */
        status = _nrf_get_reg(STATUS);
        lcd_set_cursor(1, 0);
        lcd_write_string("ST:0x");
        lcd_write_hex(status);
        if (ok) {
            lcd_write_string("  SENT  ");
        } else {
            lcd_write_string("  FAIL  ");
        }

        delay_ms(1000);
    }
}
