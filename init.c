#include <msp430.h>
#include "init.h"
#include "defines.h"
#include "global.h"
#include "spi.h"
#include "flash.h"
#include "data_proc.h"

//---------------------------------------INITIALIZATIONS---------------------------

//SYSTEM INITIALIZATION
void init_system(void)
  {
    WDTCTL = WDTPW + WDTHOLD;           // Stop watchdog timer

    ptr_vref_h = &vref_h;
    ptr_vref_l = &vref_l;
    ptr_vref_vcc = &vref_vcc;
    
    init_clk();                         // init. internal clocks
    init_pulse();                       // init. pulse for cap_meas
    init_ADC();                         // init. ADC10
    init_spi();                         // init. SPI module
    init_uart();
    init_led();                         // init. LED
    init_switch();                      // init. switch for calibration
    init_flash();                       // init. flash for write operation
    load_cal();                         // load calibration data
    __delay_cycles(128);
  }

//CLOCK INITIALIZATION
void init_clk(void)
  {
    BCSCTL1 = CALBC1_8MHZ;		//config internal clocks DCO => 100kHz
    DCOCTL = CALDCO_8MHZ;
  }

//FLASH INITIALIZATION
void init_flash(void)
  {
    while(FCTL3 & BUSY){};               // wait till timing gen is ready
    FCTL2 = FWKEY + FSSEL_2 + FN0 + FN1 + FN2 + FN4; // Clk = SMCLK/24 => 333kHz
  }

//PULSE INITIALIZATION
void init_pulse(void)
  {

    P1DIR |= BIT2;                  // config P1.2 as TA0.1
    P1SEL |= BIT2;

    TA0CCR0 = PULSE_PERIOD;         // PULSE Period
    TA0CCR1 = PULSE_WIDTH;
    TACCTL1 = OUTMOD_7;             // Toggle Mode
    TA0CTL = TASSEL_2 + MC_1;       // SMCLK, up mode
    TA0CTL &= ~TAIE;


	/*
    P1DIR |= BIT1;                  // config P1.1 as TA0.1
    P1SEL |= BIT1;

    TA0CCR0 = PULSE_PERIOD;         // PULSE Period
    TA0CCR1 = PULSE_WIDTH;
    TACCTL1 = OUTMOD_7;             // Toggle Mode
    TA0CTL = TASSEL_2 + MC_1;       // SMCLK, up mode
    TA0CTL &= ~TAIE;
    */
  }

//ADC10 INITIALIZATION
void init_ADC(void)
  {
    ADC10CTL0 &= ~ADC10IE; 				// disable interrupts on ADC10
    ADC10AE0 = BIT0 + BIT1; 			// activate analog input config
  }

//SWITCH INITIALIZATION
void init_switch()
  {
    P1DIR &= ~CAL_SW;                     // P1.0 = input
    P1REN |= CAL_SW;                      // Pullup/down resistor enabled
    P1OUT |= CAL_SW;                      // Pin is pulled enabled
    P1IES |= CAL_SW;                      // rising edge
    P1IE |= CAL_SW;                       // interrupt enable
    P1IFG &= ~CAL_SW;                     // clear interrupt flag
  }

//LED INITIALIZATION
void init_led(void)
  {
    P1DIR |= LED_GR + LED_YE;             // conifg LED output pins
    P1OUT &= ~LED_GR + ~LED_YE;           // turn off LEDs
  }

//SPI INITIALIZATION
void init_spi(void)
{
	//CS
	P4DIR |= CS1;
    P4SEL &= ~CS1;

    P3SEL |= 0x0C;                            		// P3.3,2 USCI_B0 option select
    UCB0CTL0 |= UCMSB + UCMST + UCSYNC;       		// 3-pin, 8-bit SPI mstr, MSB 1st
    UCB0CTL1 |= UCSSEL_2;                     		// SMCLK
    UCB0BR0 = 0x02;
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST;                     		// **Initialize USCI state machine**

  }

void init_uart(void){
	P3SEL |= 0x30;                       			// P3.4,5 = USCI_A0 TXD/RXD
	UCA0CTL1 |= UCSSEL_2;                     		// SMCLK
	UCA0BR0 = 0x41;                            		// 8MHz 9600
	UCA0BR1 = 0x03;                              	// 8MHz 9600
	UCA0MCTL = UCBRS0;                        		// Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     		// **Initialize USCI state machine**
	IE2 |= UCA0RXIE;                          		// Enable USCI_A0 RX interrupt
}

//LOAD CALIBRATION DATA
void load_cal(void)
  {
    unsigned char spi_volt_data;
    unsigned char *ptr_spi_volt_data;
    ptr_spi_volt_data = &spi_volt_data;

    // Set Vref+ to max to be able to use internal Vref2.5
    char maxOUT = 255;                       		// set Vref+ to Vcc
    spi_send(DAC_VREF_H, maxOUT);

    vref_l = read_flash_ref(FLASH_VREF_L);         // load calibration
    vref_h = read_flash_ref(FLASH_VREF_H);         // load calibration
    vref_vcc = read_flash_ref(FLASH_VCC);          // load calibration

  }

