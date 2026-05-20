#include "UART.h"

void initUART(void)
{
	DDRD &= ~(1<<DDD0);     // PD0 RX = entrada 
	DDRD |=  (1<<DDD1);     // PD1 TX = salida  

	UCSR0A = 0;
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);  // RX, TX, interrupción RX 
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);             // 8 bits, async           
	UBRR0  = 103;                                  // 9600 baudrate  
}

void writeChar(char caracter)
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = caracter;
}

void writeString(const char *string)
{
	for(uint8_t i = 0; string[i] != '\0'; i++)
	writeChar(string[i]);
}
