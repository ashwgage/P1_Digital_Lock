#ifndef LCD_H
#define LCD_H

#include "stm32l476xx.h"
#include <stdint.h>

/*
 * Function prototypes
 */
void LCD_init(void);
void LCD_command(uint8_t command);
void LCD_write_char(uint8_t letter);

#endif
