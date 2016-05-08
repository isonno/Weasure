/*****************************************************************
 * Driver for M16C/62P UART
 *
 * Based on main_uart.c demo by Renesas
 *****************************************************************/

#ifndef UART_H
#define UART_H

/* Function Prototypes */
void uart_text_write (_far char * msg_string);
void mcu_init(void);
void uart_init(int baud_rate);
char uart_get_char();

#define NO_DATA (-1)

#define BAUD_RATE	19200

#endif
