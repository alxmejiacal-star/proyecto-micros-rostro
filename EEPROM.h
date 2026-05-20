#ifndef EEPROM_H_
#define EEPROM_H_
#include <avr/io.h>


#define EEPROM_COUNT_DIR    0
#define EEPROM_MAX_POS      10

// Dirección base de cada banco
#define EEPROM_BANK_S0      1
#define EEPROM_BANK_S1      11
#define EEPROM_BANK_S2      21
#define EEPROM_BANK_S3      31

void    EEPROM_write(uint16_t direccion, uint8_t dato);
uint8_t EEPROM_read(uint16_t direccion);
void    EEPROM_guardar_posicion(uint8_t pos[4]);
void    EEPROM_leer_posicion(uint8_t index, uint8_t pos[4]);
uint8_t EEPROM_get_count(void);
void    EEPROM_reset(void);

#endif // EEPROM_H_
