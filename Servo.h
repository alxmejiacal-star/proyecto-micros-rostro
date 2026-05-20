#ifndef SERVO_H_
#define SERVO_H_

#include <avr/io.h>



#define SERVO_MIN   2000
#define SERVO_MAX   4000
#define SERVO_MID   3000

void     Servo_init(void);
void     Servo_set(uint8_t servo, uint16_t pulso);
uint16_t Servo_ADC_to_pulso(uint16_t adc_val);

#endif // SERVO_H_
