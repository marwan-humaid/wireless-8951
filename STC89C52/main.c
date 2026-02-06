#include <reg52.h>

sbit LED = P2^0;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 200; j++);
}

void main(void) {
    while (1) {
        LED = 0;       /* LED on  (active low) */
        delay_ms(100);
        LED = 1;       /* LED off */
        delay_ms(100);
    }
}
