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
unsigned char last_seq = 0xFF;
unsigned int pkt_count = 0;
unsigned int dup_count = 0;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 200; j++) ;
}

/* UART at 57600 baud (11.0592 MHz crystal, SMOD=1, TH1=0xFF) */
void uart_init(void) {
    SCON = 0x50;   /* mode 1, REN=1 */
    TMOD &= 0x0F;
    TMOD |= 0x20;  /* Timer 1 mode 2 (auto-reload) */
    TH1 = 0xFF;    /* 57600 baud with SMOD=1 */
    TL1 = 0xFF;
    PCON |= 0x80;  /* SMOD = 1 */
    TR1 = 1;
}

void uart_send(unsigned char c) {
    SBUF = c;
    while (!TI) ;
    TI = 0;
}

void uart_print(char *s) {
    while (*s)
        uart_send(*s++);
}

void uart_hex(unsigned char v) {
    unsigned char nibble;
    nibble = v >> 4;
    uart_send(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    nibble = v & 0x0F;
    uart_send(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
}

void uart_dec(unsigned int v) {
    unsigned char buf[5];
    unsigned char i = 0;
    if (v == 0) { uart_send('0'); return; }
    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i > 0) uart_send(buf[--i]);
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

    uart_init();
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

    /* Enter RX mode and stay there */
    _nrf_set_reg_mb(RX_ADDR_P0, addr, 5);
    _nrf_set_reg(CONFIG, NRF_CONFIG|((1<<PWR_UP)|(1<<PRIM_RX)));
    CSN = 0; { _nrf_rw(FLUSH_RX); } CSN = 1;
    _nrf_set_reg(STATUS, (1<<RX_DR));
    CE = 1;

    while (1) {
        if (_nrf_get_reg(STATUS) & (1<<RX_DR)) {
            _nrf_set_reg(STATUS, (1<<RX_DR));

            /* Check FIFO before reading (empty FIFO returns garbage) */
            while (!(_nrf_get_reg(FIFO_STATUS) & (1<<RX_EMPTY))) {
                _nrf_read_rx_payload(rx_buf);
                pkt_count++;
                count = (unsigned char)rx_buf[0];
                uart_print("PKT seq=");
                uart_hex((unsigned char)rx_buf[31]);
                uart_print(" cnt=");
                uart_hex(count);
                if ((unsigned char)rx_buf[31] == last_seq) {
                    dup_count++;
                    uart_print(" DUP\r\n");
                    continue;
                }
                last_seq = (unsigned char)rx_buf[31];
                if (count == 0 || count > 30) {
                    uart_print(" BAD\r\n");
                    continue;
                }
                uart_print(" data=");
                for (i = 0; i < count; i++) {
                    uart_send((unsigned char)rx_buf[i + 1]);
                    process_char((unsigned char)rx_buf[i + 1]);
                }
                uart_print("\r\n");
            }
        }
    }
}
