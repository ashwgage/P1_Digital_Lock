#ifndef KEYPAD_H
#define KEYPAD_H

#include "main.h"
#include <stdint.h>

/*
 * Keypad library (matrix scan)
 * ----------------------------
 * Rows    : PC4 - PC7  (inputs, pull-down)
 * Columns : PC8 - PC10 (outputs)
 *
 * Keys are returned as ASCII characters:
 *   '0'-'9', '*', '#'
 *
 * KEYPAD_NONE (0) is returned when no key is pressed.
 */

#define KEYPAD_NONE   ((char)0)
#define KEYPAD_STAR   '*'
#define KEYPAD_POUND  '#'

/* Configure the GPIO pins used by the keypad. */
void keypad_init(void);

/*
 * Non-blocking scan.
 * Returns the ASCII character for the pressed key, or
 * KEYPAD_NONE when nothing is pressed.
 */
char keypad_scan(void);

/*
 * Blocking read with debounce.
 * Waits until a key is pressed, then waits for release,
 * and returns the ASCII character of that key.
 */
char keypad_get_key(void);

#endif /* KEYPAD_H */
