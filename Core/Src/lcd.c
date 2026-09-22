#include "lcd.h"

#define LCD_FUNCTION_SET 0x28U
#define LCD_DISPLAY_ON 0x0CU
#define LCD_ENTRY_MODE 0x06U
#define LCD_CLEAR_DISPLAY 0x01U
#define LCD_DISPLAY_OFF 0x08U
#define LCD_LINE_1 0x80U
#define LCD_LINE_2 0xC0U
#define LCD_COLUMNS 20U
#define LCD_RS_PIN 0U
#define LCD_RW_PIN 1U
#define LCD_E_PIN 2U
#define LCD_DB4_PIN 4U
#define LCD_DB5_PIN 5U
#define LCD_DB6_PIN 6U
#define LCD_DB7_PIN 7U
#define LCD_DELAY_500US 2000U
#define LCD_DELAY_2MS 8000U
#define LCD_DELAY_10MS 40000U
#define LCD_DELAY_100MS 400000U

static void LCD_delay(volatile uint32_t count);
static void LCD_write_nibble(uint8_t nibble);

/* -----------------------------------
 * Simple software delay
 * ----------------------------------- */
static void LCD_delay(volatile uint32_t count) {
  while (count--) {
    __NOP();
  }
}

/* -----------------------------------
 * Send one command to LCD
 * ----------------------------------- */
void LCD_command(uint8_t command) {
  /*
   * RS = 0: instruction mode
   * R/W = 0: write mode
   */
  GPIOB->BRR = (1UL << LCD_RS_PIN) | (1UL << LCD_RW_PIN);

  /* Send bits 7-4 first. */
  LCD_write_nibble(command >> 4);

  /* Send bits 3-0 second. */
  LCD_write_nibble(command & 0x0FU);

  /*
   * Clear display and return home require
   * more time than ordinary commands.
   */
  if (command == 0x01U || command == 0x02U) {
    LCD_delay(LCD_DELAY_2MS);
  } else {
    LCD_delay(LCD_DELAY_500US);
  }
}

/* -----------------------------------
 * Send one character to LCD
 * ----------------------------------- */
void LCD_write_char(uint8_t letter) {
  /*
   * RS = 1: data mode
   * R/W = 0: write mode
   */
  GPIOB->BSRR = (1UL << LCD_RS_PIN);
  GPIOB->BRR = (1UL << LCD_RW_PIN);

  /* Send bits 7-4 first. */
  LCD_write_nibble(letter >> 4);

  /* Send bits 3-0 second. */
  LCD_write_nibble(letter & 0x0FU);

  /* Wait for the LCD to process the character. */
  LCD_delay(LCD_DELAY_500US);
}

/* -----------------------------------
 * Write a null-terminated string
 * ----------------------------------- */
void LCD_write_string(const char *text)
{
    while (*text != '\0') {
        LCD_write_char((uint8_t)*text);
        text++;
    }
}

/* -----------------------------------
 * Clear the LCD
 * ----------------------------------- */
void LCD_clear(void)
{
    LCD_command(LCD_CLEAR_DISPLAY);
}

/* -----------------------------------
 * Set the LCD cursor position
 *
 * row 0 = first row
 * row 1 = second row
 * ----------------------------------- */
void LCD_set_cursor(uint8_t row, uint8_t column)
{
    if (column >= LCD_COLUMNS) {
        column = LCD_COLUMNS - 1U;
    }

    if (row == 0U) {
        LCD_command(LCD_LINE_1 + column);
    } else {
        LCD_command(LCD_LINE_2 + column);
    }
}

static void LCD_write_nibble(uint8_t nibble) {
  /* Clear DB4-DB7 without affecting the control pins. */
  GPIOB->BRR = (1UL << LCD_DB4_PIN) | (1UL << LCD_DB5_PIN) |
               (1UL << LCD_DB6_PIN) | (1UL << LCD_DB7_PIN);

  /* Shifts nibble bits 0–3 into GPIO bits 4–7,. */
  GPIOB->BSRR = ((uint32_t)(nibble & 0x0FU) << LCD_DB4_PIN);

  /* Pulse E to latch the nibble. */
  GPIOB->BSRR = (1UL << LCD_E_PIN);
  LCD_delay(LCD_DELAY_500US);

  GPIOB->BRR = (1UL << LCD_E_PIN);
  LCD_delay(LCD_DELAY_500US);
}

/* -----------------------------------
 * Initialize LCD
 * ----------------------------------- */
void LCD_init(void) {
  /* Enable GPIOB clock */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

  /*
   * PB0 = RS
   * PB1 = R/W
   * PB2 = E
   * PB4 = DB4
   * PB5 = DB5
   * PB6 = DB6
   * PB7 = DB7
   *
   * Configure all as GPIO outputs.
   */
  GPIOB->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1 | GPIO_MODER_MODE2 |
                    GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 |
                    GPIO_MODER_MODE7);

  GPIOB->MODER |=
      (GPIO_MODER_MODE0_0 | GPIO_MODER_MODE1_0 | GPIO_MODER_MODE2_0 |
       GPIO_MODER_MODE4_0 | GPIO_MODER_MODE5_0 | GPIO_MODER_MODE6_0 |
       GPIO_MODER_MODE7_0);

  /*
   * Push-pull outputs.
   */
  GPIOB->OTYPER &=
      ~(GPIO_OTYPER_OT0 | GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT4 |
        GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);

  /*
   * No pull-up / pull-down.
   */
  GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1 | GPIO_PUPDR_PUPD2 |
                    GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 |
                    GPIO_PUPDR_PUPD7);

  /*
   * Initial control and data state:
   * command mode, write mode, enable low, data lines low.
   */
  GPIOB->BRR = (1UL << LCD_RS_PIN) | (1UL << LCD_RW_PIN) | (1UL << LCD_E_PIN) |
               (1UL << LCD_DB4_PIN) | (1UL << LCD_DB5_PIN) |
               (1UL << LCD_DB6_PIN) | (1UL << LCD_DB7_PIN);

  /*
   * Power-up delay.
   */
  LCD_delay(LCD_DELAY_100MS);

  /*
   * Wake-up sequence.
   */
  LCD_write_nibble(0x03U);
  LCD_delay(LCD_DELAY_10MS);

  LCD_write_nibble(0x03U);
  LCD_delay(LCD_DELAY_10MS);

  LCD_write_nibble(0x03U);
  LCD_delay(LCD_DELAY_500US);

  /*
   * Select 4-bit mode.
   */
  LCD_write_nibble(0x02U);
  LCD_delay(LCD_DELAY_500US);

  /*
   * Standard configuration commands.
   */
  LCD_command(LCD_FUNCTION_SET);
  LCD_command(LCD_DISPLAY_OFF);
  LCD_command(LCD_CLEAR_DISPLAY);
  LCD_command(LCD_ENTRY_MODE);
  LCD_command(LCD_DISPLAY_ON);
}
