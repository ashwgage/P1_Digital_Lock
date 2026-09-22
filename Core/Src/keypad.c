#include "keypad.h"

/* -----------------------------------------------------------------
 * Keypad wiring
 * -----------------------------------------------------------------
 * Rows    : PC4 - PC7  (inputs with pull-down resistors)
 * Columns : PC8 - PC10 (push-pull outputs)
 *
 * Scan method:
 *   Drive one column HIGH at a time and read the four rows.
 *   A pressed key connects its column to its row, so the row
 *   input reads HIGH. Pull-downs keep un-driven rows LOW.
 * ----------------------------------------------------------------- */

#define NUM_OF_ROWS 4U
#define NUM_OF_COLS 3U

#define ROW_PORT GPIOC
#define COL_PORT GPIOC

/* Keypad rows: PC4-PC7 */
#define ROW_PINS (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)
#define ROW_FIRST_PIN 4U

/* Keypad columns: PC8-PC10 */
#define COL_PINS (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10)
#define COL_FIRST_PIN 8U

/* Simple debounce delay (software loop). */
#define KEYPAD_DEBOUNCE 20000U

/*
 * ASCII key map matching the physical keypad layout.
 */
static const char key_map[NUM_OF_ROWS][NUM_OF_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

static void keypad_delay(volatile uint32_t count);
static uint8_t read_rows(void);
static int8_t row_to_index(uint8_t row_bits);

/* -----------------------------------------------------------------
 * Software delay
 * ----------------------------------------------------------------- */
static void keypad_delay(volatile uint32_t count) {
  while (count--) {
    __NOP();
  }
}

/* -----------------------------------------------------------------
 * Configure LED-free keypad GPIO pins
 * ----------------------------------------------------------------- */
void keypad_init(void) {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

  /* PC4-PC7: keypad row inputs */
  ROW_PORT->MODER &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 |
                       GPIO_MODER_MODE7);

  /* Clear pull config, then set pull-down (10) on each row. */
  ROW_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 |
                       GPIO_PUPDR_PUPD7);
  ROW_PORT->PUPDR |= (GPIO_PUPDR_PUPD4_1 | GPIO_PUPDR_PUPD5_1 |
                      GPIO_PUPDR_PUPD6_1 | GPIO_PUPDR_PUPD7_1);

  /* PC8-PC10: keypad column outputs */
  COL_PORT->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9 | GPIO_MODER_MODE10);
  COL_PORT->MODER |=
      (GPIO_MODER_MODE8_0 | GPIO_MODER_MODE9_0 | GPIO_MODER_MODE10_0);

  COL_PORT->OTYPER &= ~(GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9 | GPIO_OTYPER_OT10);

  COL_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD9 | GPIO_PUPDR_PUPD10);

  /* Columns start low */
  COL_PORT->BRR = COL_PINS;
}

/* -----------------------------------------------------------------
 * Read PC4-PC7 and shift the rows down to bits 0-3
 * ----------------------------------------------------------------- */
static uint8_t read_rows(void) {
  return (uint8_t)((ROW_PORT->IDR & ROW_PINS) >> ROW_FIRST_PIN);
}

/* -----------------------------------------------------------------
 * Convert a one-hot row reading into row number 0-3
 * ----------------------------------------------------------------- */
static int8_t row_to_index(uint8_t row_bits) {
  int8_t row = -1;

  if (row_bits == 0x01U) {
    row = 0;
  } else if (row_bits == 0x02U) {
    row = 1;
  } else if (row_bits == 0x04U) {
    row = 2;
  } else if (row_bits == 0x08U) {
    row = 3;
  }

  return row;
}

/* -----------------------------------------------------------------
 * Non-blocking scan.
 * Returns the ASCII character of the pressed key, or KEYPAD_NONE.
 * ----------------------------------------------------------------- */
char keypad_scan(void) {
  uint8_t column;
  uint8_t row_bits;
  int8_t row;

  /* Stage 1: drive all columns high; is anything pressed at all? */
  COL_PORT->BSRR = COL_PINS;
  row_bits = read_rows();

  if (row_bits == 0U) {
    COL_PORT->BRR = COL_PINS;
    return KEYPAD_NONE;
  }

  /* Stage 2: test one column at a time to locate the key. */
  COL_PORT->BRR = COL_PINS;

  for (column = 0U; column < NUM_OF_COLS; column++) {
    COL_PORT->BSRR = (1UL << (COL_FIRST_PIN + column));

    row_bits = read_rows();

    COL_PORT->BRR = (1UL << (COL_FIRST_PIN + column));

    if (row_bits != 0U) {
      row = row_to_index(row_bits);

      if (row != -1) {
        return key_map[row][column];
      }
    }
  }

  return KEYPAD_NONE;
}

/* -----------------------------------------------------------------
 * Blocking read with debounce.
 *
 * 1. Wait for a stable key press.
 * 2. Capture the key.
 * 3. Wait for release so one press yields one character.
 * ----------------------------------------------------------------- */
char keypad_get_key(void) {
  char key;

  /* Wait for a press. */
  do {
    key = keypad_scan();
  } while (key == KEYPAD_NONE);

  /* Debounce the press. */
  keypad_delay(KEYPAD_DEBOUNCE);

  /* Wait for release (all columns high, no row active). */
  COL_PORT->BSRR = COL_PINS;
  while (read_rows() != 0U) {
    /* hold until the user lets go */
  }
  COL_PORT->BRR = COL_PINS;

  /* Debounce the release. */
  keypad_delay(KEYPAD_DEBOUNCE);

  return key;
}
