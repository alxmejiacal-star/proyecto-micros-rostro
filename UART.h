#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <avr/interrupt.h>

void initUART(void);
void writeChar(char caracter);
void writeString(const char *string);

#endif // UART_H_ 
