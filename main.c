#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>

#include "UARTlib/UART.h"
#include "EEPROMlib/EEPROM.h"
#include "Servolib/Servo.h"

// ============================================================
//  MODOS DE OPERACION
// ============================================================
typedef enum {
	MODO_MANUAL = 0,
	MODO_EEPROM,
	MODO_UART,
	MODO_COUNT
} Modo_t;

volatile Modo_t modo_actual = MODO_MANUAL;

// ============================================================
//  BUFFER UART
// ============================================================
#define UART_BUF_SIZE 16
volatile char    uart_buf[UART_BUF_SIZE];
volatile uint8_t uart_idx  = 0;
volatile uint8_t cmd_listo = 0;

// Posicion actual de los 4 servos (0-255)
uint8_t pos_actual[4] = {127, 127, 127, 127};

// Indice de banco EEPROM para navegacion manual
uint8_t eeprom_idx = 0;

// ============================================================
//  TIMER0 — millis()
// ============================================================
volatile uint32_t sys_time = 0;

void Timer0_init(void)
{
	TCCR0A = (1<<WGM01);
	TCCR0B = (1<<CS01)|(1<<CS00);
	OCR0A  = 249;
	TIMSK0 |= (1<<OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
	sys_time++;
}

uint32_t millis(void)
{
	uint32_t t;
	cli();
	t = sys_time;
	sei();
	return t;
}

// ============================================================
//  ISR UART RX
// ============================================================
ISR(USART_RX_vect)
{
	char c = UDR0;

	if(c == '\n' || c == '\r')
	{
		if(uart_idx > 0)
		{
			uart_buf[uart_idx] = '\0';
			cmd_listo = 1;
		}
	}
	else if(uart_idx < UART_BUF_SIZE - 1 && cmd_listo == 0)
	{
		uart_buf[uart_idx++] = c;
	}
}

// ============================================================
//  PROTOTIPOS
// ============================================================
void     ADC_init(void);
uint16_t ADC_read(uint8_t canal);
void     GPIO_init(void);
void     LED_set_modo(Modo_t modo);
void     procesar_comando_uart(const char* cmd);
void     revisar_botones(void);
void     mostrar_banco_eeprom(uint8_t idx);

// ============================================================
//  UTILIDADES
// ============================================================

uint16_t pos_a_pulso(uint8_t pos)
{
	return SERVO_MIN + (uint16_t)((uint32_t)pos * (SERVO_MAX - SERVO_MIN) / 255);
}

static uint8_t u8_to_str(uint8_t val, char* out)
{
	uint8_t d2 = val / 100;
	uint8_t d1 = (val % 100) / 10;
	uint8_t d0 = val % 10;
	uint8_t i  = 0;
	if(d2)        out[i++] = '0' + d2;
	if(d2 || d1)  out[i++] = '0' + d1;
	out[i++] = '0' + d0;
	out[i]   = '\0';
	return i;
}

// ============================================================
//  MOSTRAR BANCO EEPROM
// ============================================================
void mostrar_banco_eeprom(uint8_t idx)
{
	uint8_t count = EEPROM_get_count();

	if(count == 0)
	{
		writeString("EEPROM vacia\r\n");
		return;
	}

	if(idx >= count) idx = 0;

	uint8_t pos[4];
	EEPROM_leer_posicion(idx, pos);

	for(uint8_t i = 0; i < 4; i++)
	Servo_set(i, pos_a_pulso(pos[i]));

	for(uint8_t i = 0; i < 4; i++)
	pos_actual[i] = pos[i];

	char buf[16];
	uint8_t bi = 0;

	writeString("BANCO ");
	bi += u8_to_str(idx + 1, buf + bi);
	buf[bi++] = '/';
	bi += u8_to_str(count, buf + bi);
	buf[bi++] = ' ';
	buf[bi++] = '|';
	buf[bi++] = ' ';
	buf[bi]   = '\0';
	writeString(buf);

	for(uint8_t i = 0; i < 4; i++)
	{
		char servo_str[10];
		uint8_t si = 0;
		servo_str[si++] = 'S';
		servo_str[si++] = '0' + i;
		servo_str[si++] = ':';
		si += u8_to_str(pos[i], servo_str + si);
		servo_str[si++] = ' ';
		servo_str[si]   = '\0';
		writeString(servo_str);
	}
	writeString("\r\n");

	PORTD ^= (1<<PORTD7);
}

// ============================================================
//  MAIN
// ============================================================
int main(void)
{
	cli();
	GPIO_init();
	ADC_init();
	Servo_init();
	initUART();
	Timer0_init();
	sei();

	writeString("================================\r\n");
	writeString("  Sistema listo para empezar\r\n");
	writeString("================================\r\n");
	writeString("Modo actual: MANUAL\r\n\r\n");
	writeString("MODOS:\r\n");
	writeString("  1. MANUAL  - Mueve potenciometros\r\n");
	writeString("  2. EEPROM  - Reproduce memoria\r\n");
	writeString("  3. UART    - Comandos por terminal\r\n\r\n");
	writeString("COMANDOS UART:\r\n");
	writeString("  S0:128  - Mover servo (0-3) a valor (0-255)\r\n");
	writeString("  SAVE    - Guardar posicion actual en EEPROM\r\n");
	writeString("  PLAY    - Reproducir secuencia EEPROM un banco a la ves\r\n");
	writeString("  CLEAR   - Borrar toda la EEPROM\r\n");
	writeString("  MODE:0  - Cambiar a modo MANUAL\r\n");
	writeString("  MODE:1  - Cambiar a modo EEPROM\r\n");
	writeString("  MODE:2  - Cambiar a modo UART\r\n");
	writeString("================================\r\n\r\n");

	while(1)
	{
		revisar_botones();

		if(cmd_listo)
		{
			char cmd_copia[UART_BUF_SIZE];
			cli();
			memcpy(cmd_copia, (const char*)uart_buf, UART_BUF_SIZE);
			cmd_listo = 0;
			uart_idx  = 0;
			sei();

			procesar_comando_uart(cmd_copia);
		}

		switch(modo_actual)
		{
			case MODO_MANUAL:
			{
				static uint32_t last_adc_time = 0;
				if(millis() - last_adc_time >= 20)
				{
					last_adc_time = millis();
					for(uint8_t i = 0; i < 4; i++)
					{
						uint16_t adc  = ADC_read(i);
						Servo_set(i, Servo_ADC_to_pulso(adc));
						pos_actual[i] = (uint8_t)(adc >> 2);
					}
				}
			}
			break;

			case MODO_EEPROM:
			break;

			case MODO_UART:
			break;

			default:
			break;
		}
	}
}

// ============================================================
//  REVISAR BOTONES
// ============================================================
void revisar_botones(void)
{
	static uint32_t btn_modo_time  = 0;
	static uint32_t btn_guard_time = 0;
	static uint8_t  btn_modo_ant   = 1;
	static uint8_t  btn_guard_ant  = 1;

	uint32_t ahora = millis();

	// --- Boton de modo ---
	uint8_t btn_modo = (PIND >> PD4) & 1;

	if(btn_modo_ant == 1 && btn_modo == 0 && (ahora - btn_modo_time) > 50)
	{
		btn_modo_time = ahora;
		modo_actual   = (modo_actual + 1) % MODO_COUNT;
		LED_set_modo(modo_actual);

		if(modo_actual == MODO_MANUAL) writeString("MODO: MANUAL\r\n");
		if(modo_actual == MODO_EEPROM)
		{
			eeprom_idx = 0;
			writeString("MODO: EEPROM\r\n");
			writeString("BTN GUARD / PLAY = avanzar banco\r\n");
		}
		if(modo_actual == MODO_UART) writeString("MODO: UART\r\n");
	}
	btn_modo_ant = btn_modo;

	// --- Boton guardar / navegar ---
	uint8_t btn_guard = (PIND >> PD5) & 1;

	if(btn_guard_ant == 1 && btn_guard == 0 && (ahora - btn_guard_time) > 50)
	{
		btn_guard_time = ahora;

		if(modo_actual == MODO_MANUAL)
		{
			// Comportamiento original: guardar posicion en EEPROM
			EEPROM_guardar_posicion(pos_actual);
			PORTD |= (1<<PORTD7);
			writeString("Posicion guardada\r\n");
		}
		else if(modo_actual == MODO_EEPROM)
		{
			// Mostrar banco actual y luego avanzar
			uint8_t count = EEPROM_get_count();
			if(count > 0)
			{
				mostrar_banco_eeprom(eeprom_idx);
				eeprom_idx = (eeprom_idx + 1) % count;
			}
			else
			{
				writeString("EEPROM vacia\r\n");
			}
		}
	}

	// Apagar LED de confirmacion en modo manual
	if(modo_actual == MODO_MANUAL &&
	(PORTD & (1<<PORTD7)) &&
	(ahora - btn_guard_time) > 200)
	{
		PORTD &= ~(1<<PORTD7);
	}

	btn_guard_ant = btn_guard;
}

// ============================================================
//  PROCESAR COMANDO UART
// ============================================================
void procesar_comando_uart(const char* cmd)
{
	writeString("RX: ");
	writeString(cmd);
	writeString("\r\n");
	LED_set_modo(MODO_UART);

	// Mover servo individual: "S0:128"
	if(cmd[0] == 'S' && cmd[1] >= '0' && cmd[1] <= '3' && cmd[2] == ':')
	{
		uint8_t servo = cmd[1] - '0';
		uint8_t valor = (uint8_t)atoi(cmd + 3);
		pos_actual[servo] = valor;
		Servo_set(servo, pos_a_pulso(valor));
		writeString("OK\r\n");
	}
	// PLAY — avanza al siguiente banco
	else if(strncmp(cmd, "PLAY", 4) == 0)
	{
		if(modo_actual == MODO_EEPROM)
		{
			uint8_t count = EEPROM_get_count();
			if(count == 0)
			{
				writeString("EEPROM vacia\r\n");
			}
			else
			{
				mostrar_banco_eeprom(eeprom_idx);
				eeprom_idx = (eeprom_idx + 1) % count;
			}
		}
		else
		{
			// Entrar a EEPROM y mostrar primer banco
			modo_actual = MODO_EEPROM;
			LED_set_modo(MODO_EEPROM);
			eeprom_idx = 0;
			writeString("MODO: EEPROM\r\n");
			uint8_t count = EEPROM_get_count();
			if(count > 0)
			{
				mostrar_banco_eeprom(eeprom_idx);
				eeprom_idx = (eeprom_idx + 1) % count;
			}
			else
			{
				writeString("EEPROM vacia\r\n");
			}
		}
	}
	// Guardar posicion
	else if(strncmp(cmd, "SAVE", 4) == 0)
	{
		EEPROM_guardar_posicion(pos_actual);
		writeString("Guardado\r\n");
	}
	// Borrar EEPROM
	else if(strncmp(cmd, "CLEAR", 5) == 0)
	{
		EEPROM_reset();
		eeprom_idx = 0;
		writeString("EEPROM borrada\r\n");
	}
	// Cambio de modo remoto
	else if(strncmp(cmd, "MODE:0", 6) == 0)
	{
		modo_actual = MODO_MANUAL;
		LED_set_modo(MODO_MANUAL);
		writeString("MODO: MANUAL\r\n");
	}
	else if(strncmp(cmd, "MODE:1", 6) == 0)
	{
		modo_actual = MODO_EEPROM;
		LED_set_modo(MODO_EEPROM);
		eeprom_idx = 0;
		writeString("MODO: EEPROM\r\n");
	}
	else if(strncmp(cmd, "MODE:2", 6) == 0)
	{
		modo_actual = MODO_UART;
		LED_set_modo(MODO_UART);
		writeString("MODO: UART\r\n");
	}
	else
	{
		writeString("CMD desconocido\r\n");
		writeString("Comandos: S0-S3:0-255 | SAVE | PLAY | CLEAR | MODE:0/1/2\r\n");
	}
}

// ============================================================
//  ADC
// ============================================================
void ADC_init(void)
{
	ADMUX  = (1<<REFS0);
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t ADC_read(uint8_t canal)
{
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
	return ADC;
}

// ============================================================
//  GPIO
// ============================================================
void GPIO_init(void)
{
	DDRD  |=  (1<<DDD6)|(1<<DDD7);
	PORTD &= ~((1<<PORTD6)|(1<<PORTD7));
	DDRD  &= ~((1<<DDD4)|(1<<DDD5));
	PORTD |=  ((1<<PORTD4)|(1<<PORTD5));
}

void LED_set_modo(Modo_t modo)
{
	switch(modo)
	{
		case MODO_MANUAL: PORTD &= ~(1<<PORTD6); break;
		case MODO_EEPROM: PORTD |=  (1<<PORTD6); break;
		case MODO_UART:   PORTD ^=  (1<<PORTD6); break;
		default: break;
	}
}
