/* =================================================================
 * P1 - Digital Lock
 * =================================================================
 * Integrates the LCD module and the 4x3 matrix keypad to build a
 * virtual electronic lock. The onboard LED (LD2, PA5) is the locking
 * mechanism: ON = locked, OFF = unlocked.
 *
 * Wiring
 * ------
 *   Keypad rows    : PC4 - PC7  (inputs, pull-down)
 *   Keypad columns : PC8 - PC10 (outputs)
 *
 *   LCD RS/RW/E    : PB0 / PB1 / PB2
 *   LCD DB4 - DB7  : PB4 - PB7
 *
 *   Lock LED (LD2) : PA5
 *
 * Behavior
 * --------
 *   - Powers up LOCKED with the default PIN.
 *   - LCD shows the lock status on both rows.
 *   - While entering a PIN, digits are echoed on the LCD.
 *   - '*' clears the current entry.
 *   - '#' submits the entry.
 *       * When LOCKED : a correct PIN unlocks the box.
 *       * When UNLOCKED: '#' with no digits re-locks (keeps same PIN);
 *                        entering digits then '#' reprograms the PIN.
 * ================================================================= */

#include "main.h"
#include "lcd.h"
#include "keypad.h"

/* Onboard green LED LD2 is on PA5. */
#define LED_PORT GPIOA
#define LED_PIN 5U

/* PIN constraints. */
#define PIN_MAX_LEN 8U   /* buffer capacity              */
#define PIN_MIN_LEN 4U   /* requirement: at least 4 digits */

/* PIN used when the lock first powers on. */
static const char DEFAULT_PIN[] = "1234";

typedef enum {
  STATE_LOCKED,
  STATE_UNLOCKED
} lock_state_t;

static char stored_pin[PIN_MAX_LEN + 1U];
static char entry[PIN_MAX_LEN + 1U];
static uint8_t entry_len;
static lock_state_t state;

void SystemClock_Config(void);
static void lock_led_init(void);
static void lock_led_on(void);
static void lock_led_off(void);

static void copy_pin(char *dst, const char *src);
static uint8_t pins_match(const char *a, const char *b);

static void entry_reset(void);
static void entry_add(char digit);

static void show_locked(void);
static void show_unlocked(void);
static void show_entry(void);
static void show_message(const char *line1, const char *line2);

static void enter_locked_state(void);
static void enter_unlocked_state(void);
static void handle_key(char key);

/* Main Loop */
int main(void) {
  HAL_Init();
  SystemClock_Config();

  /* Initialize the LED, LCD, and keypad. */
  lock_led_init();
  LCD_init();
  keypad_init();

  /* Start locked with the default PIN. */
  copy_pin(stored_pin, DEFAULT_PIN);
  enter_locked_state();

  while (1) {
    char key = keypad_get_key();
    handle_key(key);
  }
}

/* Set PA5 up as an output for the lock LED. */
static void lock_led_init(void) {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

  /* PA5 as push-pull output, no pull resistor. */
  LED_PORT->MODER &= ~GPIO_MODER_MODE5;
  LED_PORT->MODER |= GPIO_MODER_MODE5_0;
  LED_PORT->OTYPER &= ~GPIO_OTYPER_OT5;
  LED_PORT->PUPDR &= ~GPIO_PUPDR_PUPD5;

  LED_PORT->BRR = (1UL << LED_PIN); /* start off; state fn sets it */
}

/* Turn the lock LED on. */
static void lock_led_on(void) { 
  LED_PORT->BSRR = (1UL << LED_PIN); 
}

/* Turn the lock LED off. */
static void lock_led_off(void) { 
  LED_PORT->BRR = (1UL << LED_PIN); 
}

/* Copy a PIN string into another buffer. */
static void copy_pin(char *dst, const char *src) {
  uint8_t i = 0U;
  while (src[i] != '\0' && i < PIN_MAX_LEN) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

/* Return 1 when two PIN strings are identical, else 0. */
static uint8_t pins_match(const char *a, const char *b) {
  uint8_t i = 0U;
  while (a[i] != '\0' && b[i] != '\0') {
    if (a[i] != b[i]) {
      return 0U;
    }
    i++;
  }
  return (a[i] == '\0' && b[i] == '\0') ? 1U : 0U;
}

/* Clear the current PIN entry. */
static void entry_reset(void) {
  entry_len = 0U;
  entry[0] = '\0';
}

/* Add one digit if the PIN is not full. */
static void entry_add(char digit) {
  if (entry_len < PIN_MAX_LEN) {
    entry[entry_len] = digit;
    entry_len++;
    entry[entry_len] = '\0';
  }
}

/* Display two lines of text on the LCD. */
static void show_message(const char *line1, const char *line2) {
  LCD_clear();
  LCD_set_cursor(0U, 0U);
  LCD_write_string(line1);
  LCD_set_cursor(1U, 0U);
  LCD_write_string(line2);
}

/* Row 0: status. Row 1: entry prompt. */
static void show_locked(void) { 
  show_message("LOCKED", "ENTER KEY:"); 
}

static void show_unlocked(void) {
  show_message("UNLOCKED", "# LOCK  * CLEAR");
}

/* Update the current PIN entry on the second row. */
static void show_entry(void) {
  uint8_t used;

  LCD_set_cursor(1U, 0U);
  if (state == STATE_LOCKED) {
    LCD_write_string("ENTER KEY:");
  } else {
    LCD_write_string("NEW PIN:  ");
  }
  LCD_write_char(' ');
  LCD_write_string(entry);

  /* Pad the remainder of the row so previously shown digits clear. */
  used = (uint8_t)(11U + entry_len);
  while (used < 20U) {
    LCD_write_char(' ');
    used++;
  }
}

/* Change the lock to its locked state. */
static void enter_locked_state(void) {
  state = STATE_LOCKED;
  entry_reset();
  lock_led_on();
  show_locked();
}

/* Change the lock to its unlocked state. */
static void enter_unlocked_state(void) {
  state = STATE_UNLOCKED;
  entry_reset();
  lock_led_off();
  show_unlocked();
}

/* Handle a key pressed on the keypad. */
static void handle_key(char key) {
  if (key == KEYPAD_STAR) {
    /* '*' clears whatever has been entered and restarts entry. */
    entry_reset();
    if (state == STATE_LOCKED) {
      show_locked();
    } else {
      show_unlocked();
    }
    return;
  }

  if (key == KEYPAD_POUND) {
    if (state == STATE_LOCKED) {
      /* Submit the PIN attempt. */
      if (entry_len >= PIN_MIN_LEN && pins_match(entry, stored_pin)) {
        enter_unlocked_state();
      } else {
        show_message("WRONG PIN CUH", "TRY AGAIN");
        entry_reset();
      }
    } else {
      /* UNLOCKED: '#' either re-locks or reprograms the PIN. */
      if (entry_len == 0U) {
        /* Re-lock with the same PIN. */
        enter_locked_state();
      } else if (entry_len >= PIN_MIN_LEN) {
        /* Reprogram the PIN, then return to the unlocked screen. */
        copy_pin(stored_pin, entry);
        show_message("PIN UPDATED", "# LOCK  * CLEAR");
        entry_reset();
      } else {
        show_message("PIN TOO SHORT", "MIN 4 DIGITS");
        entry_reset();
      }
    }
    return;
  }

  /* Add number keys to the current PIN entry. */
  if (key >= '0' && key <= '9') {
    entry_add(key);
    show_entry();
  }
}

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}

void Error_Handler(void) {
  __disable_irq();
  while (1) {
  }
}
