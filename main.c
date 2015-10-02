/*
 * Original Michael Oberacher Code for Soil_moisture Sensor SMS50-v1
 * - Vref change to internal 2,5V
 * - flash fixup
 * - Calculation formula change
 * - UART implementation
 * - change to MSP430F2274
 */

#include <msp430.h>
#include "defines.h"
#include "init.h"
#include "global.h"
#include "flash.h"
#include "spi.h"
#include "adc.h"
#include "data_proc.h"
#include "pulse.h"
#include "user_interf.h"
#include "measure.h"
#include "serial/serial.h"

//VARIABLES

volatile int temperature;			// temperature for compensation
volatile int *ptr_temperature;

volatile int temp_raw;				// temperature for compensation
volatile int *ptr_temp_raw;

volatile int moisture;          	// relative moisture
volatile int *ptr_mois_perc;		// mois in %

volatile char spi_data;				// converted data for SPI message
volatile char *ptr_spi_data;		// 0 - 255

unsigned int meas_mois_raw;
unsigned int *ptr_mois_raw;			// raw mois 0 - 1024

float test_mois_volt;				//test variable

//PROGRAMMCODE

void main(void)
  {
    ptr_temp_raw = &temp_raw;		// pointer allocations
    ptr_temperature = &temperature;  
    
    ptr_mois_raw = &meas_mois_raw;
    ptr_mois_perc = &moisture;
    ptr_spi_data = &spi_data;
    
    init_system();					// init. system
    __delay_cycles(128);
        
    _EINT();						// enable general interrupts
    
    // Set Vref+ to max to be able to use internal Vref2.5
    *ptr_spi_data = 255;                       	// set Vref+ to Vcc
    spi_send(DAC_VREF_H, *ptr_spi_data);

    while(1)						// continous program cycle
      { 
        /*
        //-------------TEMPERATURE-----------
        *ptr_temp_raw = read_ADC(ADC_TEMP);
        *ptr_temperature = conv_temp(*ptr_temp_raw);

        //send to output
        _DINT();
        spi_send(DAC_OUT_TEMP, *ptr_temperature);
        _EINT();
         */

        //-------------MOISTURE--------------
        *ptr_mois_raw = meas_moisture();
        //test_mois_volt = *ptr_meas_mois * ((*ptr_vref_h - *ptr_vref_l)/1024) + *ptr_vref_l;

        // calculate with calibration values
        *ptr_mois_perc = calc_mois_perc(*ptr_mois_raw);

        //*ptr_moisture = conv_mois(*ptr_meas_mois);
        char mois_out = conv_mois_dac(*ptr_mois_raw); //writeback to DAC

        //send to output
        _DINT();
        spi_send(DAC_OUT_MOIS, mois_out);
        _EINT();
      }
  }


//---------------------------------------INTERRUPTS-----------------------------------------------------------

// CALIBRATION INTERRUPT FROM SWITCH
// Port 1 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
  {
        unsigned int adc_value;
        unsigned int  *ptr_adc_value;
        ptr_adc_value = &adc_value;
        
        _DINT();                                   	// disable interrupt
                     
        *ptr_adc_value = read_ADC(ADC_VCC);        	// measure supply voltage
        
        *ptr_vref_vcc = *ptr_adc_value;  			// save VCC val
        
        // Set Vref+ to max to be able to use internal Vref2.5
        *ptr_spi_data = 255;                       	// set Vref+ to Vcc
        spi_send(DAC_VREF_H, *ptr_spi_data);

        //TODO: delete me
        /* OBSOLET
        *ptr_spi_data = 0;                         	// set VREF- to GND
        spi_send(DAC_VREF_L, *ptr_spi_data);
        *ptr_spi_data = 255;                       	// set Vref+ to Vcc
        spi_send(DAC_VREF_H, *ptr_spi_data);
        */
        
        // meas moisture with maximum delta_Volt DRY
        blink_led_poll_sw(LED_YE);
        *ptr_mois_raw = meas_moisture();
        *ptr_vref_h = (*ptr_mois_raw);				// save high reference raw
        
        confirm_led(LED_YE);
        
        // meas moisture with maximum delta_Volt MOIST
        blink_led_poll_sw(LED_GR);
        *ptr_mois_raw = meas_moisture();
        *ptr_vref_l = (*ptr_mois_raw);				// save low reference raw
        
        erase_flash(FLASH_VREF_L);
        
        write_flash_Vref(*ptr_vref_l, *ptr_vref_h, *ptr_vref_vcc); 	// write config values to info flash

        confirm_led(LED_GR);
        
        //TODO: delete me
        /*  OBSOLET
        // set references for ADC10
        *ptr_spi_data = conv_dac(*ptr_vref_l);
        spi_send(DAC_VREF_L, *ptr_spi_data);
        *ptr_spi_data = conv_dac(*ptr_vref_h);
        spi_send(DAC_VREF_H, *ptr_spi_data);
        */
        

        P2IFG &= ~CAL_SW;                     // clear interrupt flag
        _EINT();                              // enable interrupt
  }

#define _UART

#ifdef _UART
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	//TEST structure
	*ptr_mois_perc = *ptr_mois_perc < 100 ? *ptr_mois_perc+1 : 0;

	static int cmdMode = 0;
	CMD cmd;
	char in_key = UCA0RXBUF;                  // TX -> RXed character

	cmd.cmd = in_key;
	cmd.val1 = 0x00;
	cmd.val2 = 0x00;

	switch (cmd.cmd) {
	case CMD_MOIS:
		cmd.val1 = *ptr_mois_perc;
		break;
	case CMD_VOLT:
		cmd.val1 = (*ptr_mois_perc*25);
		break;
	case CMD_MIN:
		cmd.val1 = *ptr_vref_l;
		break;
	case CMD_MAX:
		cmd.val1 = *ptr_vref_h;
		break;
	case CMD_CALI:
		cmd.val1 = 1;
		break;
	case CMD_DRY:
		//*ptr_vref_l = 330; //*ptr_mois_raw;
		*ptr_vref_l = *ptr_mois_raw;
		cmd.val1 = *ptr_vref_l;
		break;
	case CMD_WET:
		//*ptr_vref_h = 970; //*ptr_mois_raw;
		*ptr_vref_h = *ptr_mois_raw;
		cmd.val1 = *ptr_vref_h;
		break;
	case CMD_FIN:
		cmd.val1 = 1;
        erase_flash(FLASH_VREF_L);
        write_flash_Vref(*ptr_vref_l, *ptr_vref_h, *ptr_vref_vcc); 	// write config values to info flash
		break;
	case CMD_TEST:
		*ptr_vref_l = 450;
		*ptr_vref_h = 1033;
		break;
	case CMD_VERS:
		cmd.val1 = VERSION;
		cmd.val2 = BUILD;
		break;
	default:
		cmd.cmd = CMD_ERROR;
		break;
	}

	send_CMD(cmd);

}
#endif

