#include <msp430.h>
#include "defines.h"
#include "global.h"

//___SPI DATA TRANSMISSION___
void spi_send(unsigned char device, unsigned char data)
  {
    int *ptr_message;                           // reset all message bits
    int message = 0;
    ptr_message = &message;
    unsigned char *ptr_data;
    ptr_data = &data;

    switch(device)                                                
      {
        case DAC_VREF_H:    P4OUT &= ~CS1;            // select DAC1, CS1 lo
                            break;
        case DAC_OUT_MOIS:  P4OUT &= ~CS1;            // select DAC1, CS1 lo
                            *ptr_message |=(1<<15);   // DAC module B
                            break;
        /*
        case DAC_OUT_MOIS:  P4OUT |= CS1;             // select DAC2, CS2 lo
                            P4OUT &= ~CS2;
                            *ptr_message |=(1 << 13); // DAC gain 1x (2.048V)
                            break;
        case DAC_OUT_TEMP:  P4OUT |= CS1;             // select DAC2, CS2 lo
                            P4OUT &= ~CS2;
                            *ptr_message |=(1<<15);   // DAC module B
                            *ptr_message |=(1 << 13); // DAC gain 1x (2.048V)
                            break;
        */
        default:            break;
      }

    *ptr_message |=(1 << 12);               // activate DAC
    *ptr_message |=(*ptr_data << 4);        // add voltage level information
    UCB0TXBUF = *ptr_message;              	// write data to send into fifo
    //USICNT |= 16;                         // reload counter
    while (!(IFG2 & UCB0TXIFG));            // USCI_B0 TX buffer ready?
    P4OUT |= CS1;                           // CSs hi

  }
