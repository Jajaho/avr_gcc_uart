#include <avr/io.h>
#include <stdint.h>
#include "stddef.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include "usart.h"
#include "fifo.h"

//static char rxBuffer[32] = "";
//static uint8_t nextFree = 0;

fifo_t fifo;    // receive ringbuffer

// strcasestr()
// char * strsep(char **stringp, const char *delim);
void parse_cmd(char *input) {    
    char *start = input;  
    // Pointer to array, will be overriden by strsep() with pointer to first char after a found delimeter, or NULL ptr. if the end of the string was reached
    char **header[MAX_DEL_COUNT];    // Array of pointers to the first char after a found delimeter
    //uint8_t delCount = 0;           // Number of delimeters found in the input string
    uint8_t newFirstIndex = 0;      // First index that the buffer will have at the end of this method
    for (uint8_t i = 0; strsep(&start, ":") && i < MAX_DEL_COUNT; i++) {
        header[i] = start;
        //delCount++;
    }
    /*
    for (uint8_t i = 0; i < delCount; i++) {
        uart_puts(header[i]);
    }
    */
    
    // match mnemonic keywords, strcasecmp returns 0 on match
    if (header[0]) {
        if (!strcasecmp(header[0], "meas") || !strcasecmp(header[0], "measure")) {
            if (header[1]) { 
                if (!strcasecmp(header[1], "ch1") || !strcasecmp(header[1], "channel1")) {
                    if (header[2]) { 
                        if (!strcasecmp(header[2], "esr")) {
                            uart_puts("Measuring ESR on CH1.");
                            // advance ringbuffer to next delimeter
                        }
                    }
                }
            }
        }
    }
    //uart_puts(rxBuffer);
}
/*
void flush_rxBuffer(void) {
    memset(&rxBuffer,0,strlen(&rxBuffer));
    nextFree = 0;
}
*/
void uart_ping(void) {
    // Wait for the data register to be empty
    while (!(UCSR0A & (1<<UDRE0))) {
        blink_twice();
    }
    UDR0 = 'x';
}

void blink(void) {
    _delay_ms(500);
    LED_ON();
    _delay_ms(200);
    LED_OFF();
    _delay_ms(500);
}

void blink_twice(void) {
    _delay_ms(500);
    LED_ON();
    _delay_ms(200);
    LED_OFF();
    _delay_ms(200);
    LED_ON();
    _delay_ms(200);
    LED_OFF();
    _delay_ms(500);
}

void blink_thrice(void) {
    _delay_ms(500);
    LED_ON();
    _delay_ms(200);
    LED_OFF();
    _delay_ms(200);
    LED_ON();
    _delay_ms(200);
    LED_OFF();
    _delay_ms(200);
    LED_ON();
    _delay_ms(200);
    LED_OFF();
    _delay_ms(500);
}

int main(void) {
    DDRB = (1 << PB5);
    fifo_data_t tmp_a[10];
    fifo_init(&fifo, tmp_a, sizeof(tmp_a) / sizeof (fifo_data_t));
    uart_init();
    sei();

    while (1)
    {
        blink();
        // ----------------------------------------------------------------------------
        // performance block access to FIFO
        // ALWAYS do it this way!
        // read 5 bytes
        uint16_t freespace, block_size, wrap_size;
        block_size = fifo_get_level(&fifo);
        wrap_size = fifo_get_read_wrap(&fifo);
        fifo_data_t tmp_b[block_size];
    
        if (fifo_get_level(&fifo) >= block_size) {
            if (block_size > wrap_size) {
                // split action into two blocks due to pointer wrap around
                memcpy(tmp_b, (uint8_t*)fifo.read_p, wrap_size * sizeof(fifo_data_t));
                fifo_read_bursted(&fifo, wrap_size);
                block_size -= wrap_size;
            }
            // no pointer wrap around in block or second half of block operation
            memcpy(tmp_b, (uint8_t*)fifo.read_p, block_size * sizeof(fifo_data_t));
            fifo_read_bursted(&fifo, block_size);
        }
        //level = fifo_get_level(&fifo);
        // ----------------------------------------------------------------------------
        char *start = (char*) tmp_b;
        uart_puts((char*) tmp_b);
        parse_cmd((char*) tmp_b);

        _delay_us(100);
    }
    
}

ISR(USART_RX_vect) {
    fifo_write_ISR(&fifo, uart_getc());
}