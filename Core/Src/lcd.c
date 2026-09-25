#include "lcd.h"

/* LCD commands for 4-bit mode. */
#define LCD_FUNCTION_SET 0x28U  /* 4-bit, 2 lines, 5x8 font        */
#define LCD_DISPLAY_ON 0x0CU    /* display on, cursor off, blink off */
#define LCD_DISPLAY_OFF 0x08U
#define LCD_ENTRY_MODE 0x06U /* increment cursor, no shift          */
#define LCD_CLEAR_DISPLAY 0x01U
#define LCD_RETURN_HOME 0x02U
#define LCD_LINE_1 0x80U /* DDRAM address, row 0                    */
#define LCD_LINE_2 0xC0U /* DDRAM address, row 1                    */
#define LCD_COLUMNS 20U

/* LCD pins connected to GPIOB. */
#define LCD_RS_PIN 0U
#define LCD_RW_PIN 1U
#define LCD_E_PIN 2U
#define LCD_DB4_PIN 4U
#define LCD_DB5_PIN 5U
#define LCD_DB6_PIN 6U
#define LCD_DB7_PIN 7U

/* Board clock speed. */
#define SYS_CLK_HZ 80000000UL

static void LCD_delay_us(uint32_t us);
static void LCD_delay_ms(uint32_t ms);
static void LCD_write_nibble(uint8_t nibble);

/* Microsecond busy-loop delay (calibrated for 80 MHz, -O0). */
static void LCD_delay_us(uint32_t us) {
  volatile uint32_t count = (SYS_CLK_HZ / 4000000UL) * us;
  while (count--) {
    __NOP();
  }
}

/* Use HAL for longer delays. */
static void LCD_delay_ms(uint32_t ms) { HAL_Delay(ms); }

/* Put one 4-bit value on DB4-DB7 and pulse the enable pin. */
static void LCD_write_nibble(uint8_t nibble) {
  /* Clear the old data bits. */
  GPIOB->BRR = (1UL << LCD_DB4_PIN) | (1UL << LCD_DB5_PIN) |
               (1UL << LCD_DB6_PIN) | (1UL << LCD_DB7_PIN);

  /* Shift nibble bits 0-3 into GPIO bits 4-7. */
  GPIOB->BSRR = ((uint32_t)(nibble & 0x0FU) << LCD_DB4_PIN);

  /* Pulse E: LCD latches data on the falling edge.
   * E must stay high >= ~450 ns; we hold it much longer for margin. */
  GPIOB->BSRR = (1UL << LCD_E_PIN);
  LCD_delay_us(5U);

  GPIOB->BRR = (1UL << LCD_E_PIN);
  LCD_delay_us(50U);
}

/* Send one command byte. */
void LCD_command(uint8_t command) {
  /* RS = 0 selects command mode. */
  GPIOB->BRR = (1UL << LCD_RS_PIN) | (1UL << LCD_RW_PIN);
  
  /* Send the byte as two nibbles. */
  LCD_write_nibble(command >> 4);
  LCD_write_nibble(command & 0x0FU);

  /* Clear-display and return-home need more time. */
  if (command == LCD_CLEAR_DISPLAY || command == LCD_RETURN_HOME) {
    LCD_delay_ms(2U);
  } else {
    LCD_delay_us(50U);
  }
}

/* Send one character to the display. */
void LCD_write_char(uint8_t letter) {
  /* RS = 1 selects data mode. */
  GPIOB->BSRR = (1UL << LCD_RS_PIN);
  GPIOB->BRR = (1UL << LCD_RW_PIN);

  LCD_write_nibble(letter >> 4);
  LCD_write_nibble(letter & 0x0FU);

  LCD_delay_us(50U);
}

/* Write characters until the string ends. */
void LCD_write_string(const char *text) {
  while (*text != '\0') {
    LCD_write_char((uint8_t)*text);
    text++;
  }
}

/* Clear the display. */
void LCD_clear(void) { 
  LCD_command(LCD_CLEAR_DISPLAY); 
}

/* Move the cursor.  row 0 = top line, row 1 = bottom line. */
void LCD_set_cursor(uint8_t row, uint8_t column) {
  if (column >= LCD_COLUMNS) {
    column = LCD_COLUMNS - 1U;
  }

  if (row == 0U) {
    LCD_command(LCD_LINE_1 + column);
  } else {
    LCD_command(LCD_LINE_2 + column);
  }
}

/* Set up GPIO and initialize the LCD. */
void LCD_init(void) {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

  /* PB0-PB2 and PB4-PB7 as GPIO outputs. */
  GPIOB->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1 | GPIO_MODER_MODE2 |
                    GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 |
                    GPIO_MODER_MODE7);
  GPIOB->MODER |=
      (GPIO_MODER_MODE0_0 | GPIO_MODER_MODE1_0 | GPIO_MODER_MODE2_0 |
       GPIO_MODER_MODE4_0 | GPIO_MODER_MODE5_0 | GPIO_MODER_MODE6_0 |
       GPIO_MODER_MODE7_0);

  /* Push-pull outputs. */
  GPIOB->OTYPER &=
      ~(GPIO_OTYPER_OT0 | GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT4 |
        GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);

  /* No pull-up / pull-down. */
  GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1 | GPIO_PUPDR_PUPD2 |
                    GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 |
                    GPIO_PUPDR_PUPD7);

  /* Start with all control/data lines low (command mode, E low). */
  GPIOB->BRR = (1UL << LCD_RS_PIN) | (1UL << LCD_RW_PIN) | (1UL << LCD_E_PIN) |
               (1UL << LCD_DB4_PIN) | (1UL << LCD_DB5_PIN) |
               (1UL << LCD_DB6_PIN) | (1UL << LCD_DB7_PIN);

  /* Power-up delay: HD44780 needs >40 ms after Vcc rises. */
  LCD_delay_ms(50U);

  /* Wake-up sequence: three 0x03 nibbles with the datasheet waits.
   *   after the 1st: >4.1 ms
   *   after the 2nd: >100 us
   *   after the 3rd: >100 us */
  LCD_write_nibble(0x03U);
  LCD_delay_ms(5U);
  LCD_write_nibble(0x03U);
  LCD_delay_us(150U);
  LCD_write_nibble(0x03U);
  LCD_delay_us(150U);

  /* Switch to 4-bit mode. */
  LCD_write_nibble(0x02U);
  LCD_delay_us(150U);

  /* Configure the display. */
  LCD_command(LCD_FUNCTION_SET);
  LCD_command(LCD_DISPLAY_OFF);
  LCD_command(LCD_CLEAR_DISPLAY);
  LCD_command(LCD_ENTRY_MODE);
  LCD_command(LCD_DISPLAY_ON);
}
