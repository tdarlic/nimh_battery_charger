#include "ch32v003fun.h"
#include <stdio.h>

// This is only for devboard
//

#define PIN_LED PD6

#define DELAYS 150

int main()
{
	SystemInit();

	// Enable GPIOs
	funGpioInitAll();
	
	funPinMode( PIN_LED,   GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );

	while(1)
	{
		// funDigitalWrite( PIN_1,     FUN_HIGH );
		// funDigitalWrite( PIN_K,     FUN_HIGH );
		funDigitalWrite( PIN_LED,   FUN_HIGH );
		// funDigitalWrite( PIN_KEVIN, FUN_HIGH );
		Delay_Ms( DELAYS );
		// funDigitalWrite( PIN_1,     FUN_LOW );
		// funDigitalWrite( PIN_K,     FUN_LOW );
		funDigitalWrite( PIN_LED,   FUN_LOW );
		// funDigitalWrite( PIN_KEVIN, FUN_LOW );
		Delay_Ms( DELAYS );
	}
}
