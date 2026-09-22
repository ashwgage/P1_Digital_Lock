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
void LCD_write_string(const char *text);
void LCD_clear(void);
void LCD_set_cursor(uint8_t row, uint8_t column);

#endif
