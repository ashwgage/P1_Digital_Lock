#ifndef KEYPAD_H
#define KEYPAD_H

#include "main.h"
#include <stdint.h>

#define KEYPAD_NONE  ((char)0)
#define KEYPAD_STAR  '*'
#define KEYPAD_POUND '#'

void keypad_init(void);
char keypad_scan(void);
char keypad_get_key(void);

#endif
