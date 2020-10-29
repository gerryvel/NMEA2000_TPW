// LED.h

#ifndef _LED_h
#define _LED_h

#include "arduino.h"

//Configuration LED
const int LEDBU = 2;

void LEDblinkslow() {
	digitalWrite(LEDBU, HIGH);   // turn the LED on (HIGH is the voltage level)
	delay(500);                       // wait
	digitalWrite(LEDBU, LOW);    // turn the LED off by making the voltage LOW
	delay(500);                       // wait
}

void LEDblinkmed() {
	digitalWrite(LEDBU, HIGH);   // turn the LED on (HIGH is the voltage level)
	delay(250);                       // wait
	digitalWrite(LEDBU, LOW);    // turn the LED off by making the voltage LOW
	delay(250);                       // wait
}

void LEDflash() {
	digitalWrite(LEDBU, HIGH);   // turn the LED on (HIGH is the voltage level)
	delay(50);                       // wait
	digitalWrite(LEDBU, LOW);    // turn the LED off by making the voltage LOW
	delay(50);                       // wait
}

void LEDoff() {
	digitalWrite(LEDBU, LOW);
}

void LEDon() {
	digitalWrite(LEDBU, HIGH);
}





#endif



