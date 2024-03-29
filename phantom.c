/* For the Phantom replacement PCB */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define bool            uint8_t
#define true            1
#define false           0
#define NULL            0
#define NA              0

#define _PINB           (uint8_t *const)&PINB
#define _PORTC          (uint8_t *const)&PORTC
#define _PORTD          (uint8_t *const)&PORTD
#define _PORTE          (uint8_t *const)&PORTE
#define _PORTF          (uint8_t *const)&PORTF

#define NROW            6
#define NCOL            17
#define NKEY            102

const uint8_t is_modifier[NKEY] = {
  true,            true,            false,           false,           false,           false,  // COL  0
  true,            false,           false,           false,           false,           false,  // COL  1
  true,            false,           false,           false,           false,           false,  // COL  2
  NA,              false,           false,           false,           false,           false,  // COL  3
  NA,              false,           false,           false,           false,           false,  // COL  4
  NA,              false,           false,           false,           false,           false,  // COL  5
  NA,              false,           false,           false,           false,           false,  // COL  6
  false,           false,           false,           false,           false,           false,  // COL  7
  NA,              false,           false,           false,           false,           false,  // COL  8
  NA,              false,           false,           false,           false,           false,  // COL  9
  true,            false,           false,           false,           false,           false,  // COL 10
  true,            false,           false,           false,           false,           false,  // COL 11
  false,           NA,              false,           false,           NA,              false,  // COL 12
  true,            true,            false,           false,           false,           false,  // COL 13

  false,           NA,              NA,              false,           false,           false,  // COL 14
  false,           false,           NA,              false,           false,           false,  // COL 15
  false,           NA,              NA,              false,           false,           false,  // COL 16
};

const uint8_t layout[NKEY] = {
//ROW 0            ROW 1            ROW 2            ROW 3            ROW 4
  KEY_LEFT_CTRL,   KEY_LEFT_SHIFT,  KEY_CAPS_LOCK,   KEY_TAB,         KEY_1,              KEY_ESC,        // COL  0
  KEY_LEFT_GUI,    KEY_PIPE,        KEY_A,           KEY_Q,           KEY_2,              KEY_TILDE,      // COL  1
  KEY_LEFT_ALT,    KEY_Z,           KEY_S,           KEY_W,           KEY_3,              KEY_F1,         // COL  2
  NA,              KEY_X,           KEY_D,           KEY_E,           KEY_4,              KEY_F2,         // COL  3
  NA,              KEY_C,           KEY_F,           KEY_R,           KEY_5,              KEY_F3,         // COL  4
  NA,              KEY_V,           KEY_G,           KEY_T,           KEY_6,              KEY_F4,         // COL  5
  NA,              KEY_B,           KEY_H,           KEY_Y,           KEY_7,              KEY_F5,         // COL  6
  KEY_SPACE,       KEY_N,           KEY_J,           KEY_U,           KEY_8,              KEY_F6,         // COL  7
  NA,              KEY_M,           KEY_K,           KEY_I,           KEY_9,              KEY_F7,         // COL  8
  NA,              KEY_COMMA,       KEY_L,           KEY_O,           KEY_0,              KEY_F8,         // COL  9
  KEY_RIGHT_ALT,   KEY_PERIOD,      KEY_SEMICOLON,   KEY_P,           KEY_MINUS,          KEY_F9,         // COL 10
  KEY_RIGHT_GUI,   KEY_SLASH,       KEY_QUOTE,       KEY_LEFT_BRACE,  KEY_EQUAL,          KEY_F10,        // COL 11
  KEY_APPLICATION, NA,              KEY_BACKSLASH,   KEY_RIGHT_BRACE, NA,                 KEY_F11,        // COL 12
  KEY_RIGHT_CTRL,  KEY_RIGHT_SHIFT, KEY_ENTER,       KEY_BACKSLASH,   KEY_BACKSPACE,      KEY_F12,        // COL 13

  KEY_LEFT,        NA,              NA,              KEY_DELETE,      KEY_INSERT,         KEY_PRINTSCREEN,// COL 14
  KEY_DOWN,        KEY_UP,          NA,              KEY_END,         KEY_HOME,           KEY_SCROLL_LOCK,// COL 15
  KEY_RIGHT,       NA,              NA,              KEY_PAGE_DOWN,   KEY_PAGE_UP,        KEY_PAUSE,      // COL 16
};

uint8_t *const row_port[NROW] = { _PINB,  _PINB,  _PINB,  _PINB,  _PINB,  _PINB};
const uint8_t   row_bit[NROW] = {  0x01,   0x02,   0x04,   0x08,   0x10,   0x20};

uint8_t *const col_port[NCOL] = {_PORTD, _PORTC, _PORTC, _PORTD, _PORTD, _PORTE,
				 _PORTF, _PORTF, _PORTF, _PORTF, _PORTF, _PORTF,
				 _PORTD, _PORTD, _PORTD, _PORTD, _PORTD};
const uint8_t   col_bit[NCOL] = {  0x20,   0x80,   0x40,   0x10,   0x01,   0x40,
				   0x01,   0x02,   0x10,   0x20,   0x40,   0x80,
				   0x80,   0x40,   0x02,   0x04,   0x08};

bool pressed[102];
uint8_t queue[7] = {255,255,255,255,255,255,255};
uint8_t mod_keys = 0;

void init(void);
void send(void);
void poll(void);
void key_press(uint8_t key_id);
void key_release(uint8_t key_id);

int main(void) {
  uint8_t row, col, key_id, i;

  init();
  for(i=0; i<NCOL; i++)
    *col_port[col] |= col_bit[i];

  for(;;) {
    _delay_ms(5);                                //  Debouncing
    for(col=0; col<NCOL; col++) {
      *col_port[col] &= ~col_bit[col];
      _delay_us(1);
      for(row=0; row<NROW; row++) {
	key_id = col*NROW+row;
	if(!(*row_port[row] & row_bit[row])) {
	  if(!pressed[key_id])
	    key_press(key_id);
	}
	else if(pressed[key_id])
	  key_release(key_id);
      }
      *col_port[col] |= col_bit[col];
    }

    //    OCR1B++; OCR1C++;

    PORTB = (PORTB & 0b00111111) | ((keyboard_leds << 5) & 0b11000000);
    DDRB  = (DDRB  & 0b00111111) | ((keyboard_leds << 5) & 0b11000000);

  }
}

inline void send(void) {
  //return;
  uint8_t i;
  for(i=0; i<6; i++)
    keyboard_keys[i] = queue[i]<255? layout[queue[i]]: 0;
  keyboard_modifier_keys = mod_keys;
  usb_keyboard_send();
}

inline void key_press(uint8_t key_id) {
  uint8_t i;
  pressed[key_id] = true;
  if(is_modifier[key_id])
    mod_keys |= layout[key_id];
  else {
    for(i=5; i>0; i--) queue[i] = queue[i-1];
    queue[0] = key_id;
  }
  send();
}

inline void key_release(uint8_t key_id) {
  uint8_t i;
  pressed[key_id] = false;
  if(is_modifier[key_id])
    mod_keys &= ~layout[key_id];
  else {
    for(i=0; i<6; i++) if(queue[i]==key_id) break;
    for(; i<6; i++) queue[i] = queue[i+1];
  }
  send();
}

void init(void) {
  uint8_t i;
  CLKPR = 0x80; CLKPR = 0;
  usb_init();
  while(!usb_configured());
  _delay_ms(1000);
  // PORTB is set as input with pull-up resistors
  // PORTC,D,E,F are set to high output
  DDRB  = 0xC0; DDRC  = 0xFF; DDRD  = 0xFF; DDRE  = 0xFF; DDRF  = 0xFF;
  PORTB = 0x3F; PORTC = 0xFF; PORTD = 0xFF; PORTE = 0xFF; PORTF = 0xFF;
  for(i=0; i<NKEY; i++) pressed[i] = false;

  // LEDs are on output compare pins OC1B OC1C
  // This activates fast PWM mode on them.
  // OCR1B sets the intensity
  TCCR1A = 0b00101001;
  TCCR1B = 0b00001001;
  OCR1B = OCR1C = 32;

  // LEDs: LED_A -> PORTB6, LED_B -> PORTB7
  DDRB  &= 0b00000000;
  PORTB &= 0b00111111;


}
