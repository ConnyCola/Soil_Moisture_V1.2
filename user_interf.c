#include <msp430.h>
#include "user_interf.h"
#include "defines.h"
#include "global.h"

void blink_led_poll_sw(unsigned char led_color)
  {
    unsigned char sw_state;
    unsigned char *ptr_sw_state;
    ptr_sw_state = &sw_state;

    P1OUT |= led_color;                      // light yellow LED
    __delay_cycles(5000000);                 // wait
    *ptr_sw_state = (P1IN & CAL_SW);
    while(!(*ptr_sw_state && !(P1IN & CAL_SW)))   // falling edge detection
      {
        *ptr_sw_state = (P1IN & CAL_SW);   //blink yellow LED
        P1OUT ^= led_color;
        __delay_cycles(5000000);
      };
  }

void confirm_led(unsigned char led_color)
  {
    P1OUT |= led_color;                        // light yellow LED
    __delay_cycles(10000000);                    // wait 1.25sec
    P1OUT &= ~led_color;                       // turn off yellow LED
  }
