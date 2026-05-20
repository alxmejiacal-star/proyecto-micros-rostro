#include "Servo.h"
#include <avr/interrupt.h>


volatile uint16_t pulses[4]  = {SERVO_MID, SERVO_MID, SERVO_MID, SERVO_MID};
volatile uint8_t  servo_idx  = 0;


//  INIT

void Servo_init(void)
{
	DDRB |=  (1<<DDB0)|(1<<DDB2)|(1<<DDB3);  // PB0 en lugar de PB1
    DDRD |=  (1<<DDD3);
    PORTB &= ~((1<<PORTB0)|(1<<PORTB2)|(1<<PORTB3));
    PORTD &= ~(1<<PORTD3);

    // Timer1 modo Normal, prescaler 8
    TCCR1A = 0;
    TCCR1B = (1<<CS11);      // prescaler 8 = tick 0.5us
    OCR1A  = TCNT1 + 100;    // primera interrupcion pronto
    TIMSK1 |= (1<<OCIE1A);
}


//  ISR TIMER1

ISR(TIMER1_COMPA_vect)
{
    uint16_t now = TCNT1;

    switch(servo_idx)
    {
		case 0:
		PORTD &= ~(1<<PORTD3);
		PORTB |=  (1<<PORTB0);    
		OCR1A  =  now + pulses[0];
		servo_idx = 1;
		break;

		case 1:
		PORTB &= ~(1<<PORTB0);    // apagar D8
		PORTB |=  (1<<PORTB2);
		OCR1A  =  now + pulses[1];
		servo_idx = 2;
		break;

        case 2:
            PORTB &= ~(1<<PORTB2);  // apagar servo 1
            PORTB |=  (1<<PORTB3);  // encender servo 2
            OCR1A  =  now + pulses[2];
            servo_idx = 3;
            break;

        case 3:
            PORTB &= ~(1<<PORTB3);  // apagar servo 2
            PORTD |=  (1<<PORTD3);  // encender servo 3
            OCR1A  =  now + pulses[3];
            servo_idx = 4;
            break;

        case 4:
            PORTD &= ~(1<<PORTD3);  // apagar servo 3
            OCR1A  =  now + (40000 - (pulses[0] + pulses[1] + pulses[2] + pulses[3]));
            servo_idx = 0;
            break;
    }
}


//  Servo_set

void Servo_set(uint8_t servo, uint16_t pulso)
{
    if(pulso < SERVO_MIN) pulso = SERVO_MIN;
    if(pulso > SERVO_MAX) pulso = SERVO_MAX;
    if(servo < 4)
    {
        cli();
        pulses[servo] = pulso;
        sei();
    }
}


//  Servo_ADC_to_pulso

uint16_t Servo_ADC_to_pulso(uint16_t adc_val)
{
    return SERVO_MIN + (uint16_t)((uint32_t)adc_val * (SERVO_MAX - SERVO_MIN) / 1023);
}
