#ifndef LCD_H
#define LCD_H

#include "main.h"
#include <stdint.h>

/*
 * LCD library (HD44780-compatible, 4-bit / nibble mode)
 * -----------------------------------------------------
 * RS  : PB0
 * R/W : PB1
 * E   : PB2
 * DB4 : PB4
 * DB5 : PB5
 * DB6 : PB6
 * DB7 : PB7
 */

void LCD_init(void);
void LCD_command(uint8_t command);
void LCD_write_char(uint8_t letter);
void LCD_write_string(const char *text);
void LCD_set_cursor(uint8_t row, uint8_t column);
void LCD_clear(void);

#endif /* LCD_H */
