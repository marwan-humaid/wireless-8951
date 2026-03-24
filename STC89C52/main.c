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
char xdata ack_buf[32];
char addr[] = {0x30, 0x30, 0x30, 0x30, 0x31};

/* Ring buffer: holds received packets for main loop to process */
#define PKT_QUEUE_SIZE 8
char xdata pkt_queue[PKT_QUEUE_SIZE][32];
volatile unsigned char pkt_head = 0;
volatile unsigned char pkt_tail = 0;

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
    SCON = 0x50;
    TMOD &= 0x0F;
    TMOD |= 0x20;
    TH1 = 0xFF;
    TL1 = 0xFF;
    PCON |= 0x80;
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
 */
void timer0_isr(void) interrupt 1 {
    unsigned char next;
    TH0 = 0xEE;
    TL0 = 0x00;

    if (!(_nrf_get_reg(STATUS) & (1<<RX_DR)))
        return;

    _nrf_set_reg(STATUS, (1<<RX_DR));

    while (!(_nrf_get_reg(FIFO_STATUS) & (1<<RX_EMPTY))) {
        next = (pkt_head + 1) % PKT_QUEUE_SIZE;
        if (next == pkt_tail) {
            CSN = 0; { _nrf_rw(FLUSH_RX); } CSN = 1;
            break;
        }
        _nrf_read_rx_payload(pkt_queue[pkt_head]);
        pkt_head = next;
    }
}

void timer0_init(void) {
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = 0xEE;
    TL0 = 0x00;
    ET0 = 1;
    TR0 = 1;
    EA = 1;
}

/* Send ACK packet for a given sequence number */
void send_ack(unsigned char seq) {
    unsigned char i;
    /* Disable timer ISR while we use the radio for TX */
    ET0 = 0;
    CE = 0;

    /* Build ACK: byte 0 = 0xFF marker, byte 31 = seq */
    for (i = 0; i < 32; i++) ack_buf[i] = 0;
    ack_buf[0] = 0xFF;
    ack_buf[31] = seq;

    nrf_send(addr, ack_buf);

    /* Return to RX mode */
    _nrf_set_reg_mb(RX_ADDR_P0, addr, 5);
    _nrf_set_reg(CONFIG, NRF_CONFIG|((1<<PWR_UP)|(1<<PRIM_RX)));
    CSN = 0; { _nrf_rw(FLUSH_RX); } CSN = 1;
    _nrf_set_reg(STATUS, (1<<RX_DR));
    CE = 1;

    ET0 = 1;
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
        lcd_clear();
        cursor_row = 0;
        cursor_col = 0;
    } else if (ch >= 0x20 && ch <= 0x7E) {
        lcd_write_char(ch);
        cursor_advance();
    }
}

void main(void) {
    unsigned char i, count, seq;
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

    /* Enter RX mode */
    _nrf_set_reg_mb(RX_ADDR_P0, addr, 5);
    _nrf_set_reg(CONFIG, NRF_CONFIG|((1<<PWR_UP)|(1<<PRIM_RX)));
    CSN = 0; { _nrf_rw(FLUSH_RX); } CSN = 1;
    _nrf_set_reg(STATUS, (1<<RX_DR));
    CE = 1;

    timer0_init();

    while (1) {
        while (pkt_tail != pkt_head) {
            pkt = pkt_queue[pkt_tail];
            pkt_tail = (pkt_tail + 1) % PKT_QUEUE_SIZE;
            pkt_count++;
            count = (unsigned char)pkt[0];
            seq = (unsigned char)pkt[31];

            /* Skip ACK packets that leaked into our RX */
            if (count == 0xFF) continue;

            if (count == 0 || count > 30) continue;

            /* ACK immediately, before any UART/LCD work */
            if (seq == last_seq) {
                dup_count++;
                send_ack(seq);
                continue;
            }
            last_seq = seq;
            send_ack(seq);

            uart_print("PKT seq=");
            uart_hex(seq);
            uart_print(" cnt=");
            uart_hex(count);
            uart_print(" data=");
            for (i = 0; i < count; i++) {
                uart_send((unsigned char)pkt[i + 1]);
                process_char((unsigned char)pkt[i + 1]);
            }
            uart_print("\r\n");
        }
    }
}
