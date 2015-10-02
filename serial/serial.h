/*
 * serial.h
 *
 *  Created on: 14.12.2013
 *      Author: Matthias
 */

#ifndef SERIAL_H_
#define SERIAL_H_
#include "../global.h"


void initSerial(void);
void xtoa(unsigned long x, const unsigned long *dp);
void puth(unsigned n);
void puts(char *);
void putc(unsigned);
void sendByte(unsigned char byte );
int printf(char *format, ...);
void send_CMD(CMD cmd);

#endif /* SERIAL_H_ */
