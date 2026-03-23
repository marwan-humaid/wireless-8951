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

/* Ring buffer: holds received packets for main loop to process */
#define PKT_QUEUE_SIZE 8
char xdata pkt_queue[PKT_QUEUE_SIZE][32];
volatile unsigned char pkt_head = 0;  /* ISR writes here */
volatile unsigned char pkt_tail = 0;  /* main loop reads here */

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

/*
 * Timer 0 ISR: polls NRF24L01 and drains FIFO into ring buffer.
 * Runs every ~5ms (11.0592 MHz, mode 1, reload 0xEE00).
 * Keeps CE HIGH throughout; SPI reads don't disturb RX mode.
 */
void timer0_isr(void) interrupt 1 {
    unsigned char next;
    /* Reload timer (mode 1, no auto-reload) */
    TH0 = 0xEE;
    TL0 = 0x00;

    if (!(_nrf_get_reg(STATUS) & (1<<RX_DR)))
        return;

    _nrf_set_reg(STATUS, (1<<RX_DR));

    while (!(_nrf_get_reg(FIFO_STATUS) & (1<<RX_EMPTY))) {
        next = (pkt_head + 1) % PKT_QUEUE_SIZE;
        if (next == pkt_tail) {
            /* Queue full, flush remaining to prevent stale data */
            CSN = 0; { _nrf_rw(FLUSH_RX); } CSN = 1;
            break;
        }
        _nrf_read_rx_payload(pkt_queue[pkt_head]);
        pkt_head = next;
    }
}

void timer0_init(void) {
    TMOD &= 0xF0;
    TMOD |= 0x01;  /* Timer 0 mode 1 (16-bit) */
    TH0 = 0xEE;    /* ~5ms at 11.0592 MHz */
    TL0 = 0x00;
    ET0 = 1;        /* Enable Timer 0 interrupt */
    TR0 = 1;        /* Start Timer 0 */
    EA = 1;         /* Global interrupt enable */
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
    char xdata *pkt;

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

    /* Start timer interrupt to poll NRF24L01 */
    timer0_init();

    while (1) {
        /* Process packets from ring buffer */
        while (pkt_tail != pkt_head) {
            pkt = pkt_queue[pkt_tail];
            pkt_tail = (pkt_tail + 1) % PKT_QUEUE_SIZE;
            pkt_count++;
            count = (unsigned char)pkt[0];

            uart_print("PKT seq=");
            uart_hex((unsigned char)pkt[31]);
            uart_print(" cnt=");
            uart_hex(count);

            if ((unsigned char)pkt[31] == last_seq) {
                dup_count++;
                uart_print(" DUP\r\n");
                continue;
            }
            last_seq = (unsigned char)pkt[31];
            if (count == 0 || count > 30) {
                uart_print(" BAD\r\n");
                continue;
            }
            uart_print(" data=");
            for (i = 0; i < count; i++) {
                uart_send((unsigned char)pkt[i + 1]);
                process_char((unsigned char)pkt[i + 1]);
            }
            uart_print("\r\n");
        }
    }
}
