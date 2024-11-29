#include "ch32v003_GPIO_branchless.h"
#include <stdint.h>
#include <stdio.h>

// This is only for devboard
//
uint16_t adc1, adc2, adc3, adc4 = 0;

#define PIN_LED PD6

#define DELAYS 150

int main()
{
	SystemInit();

	GPIO_port_enable(GPIO_port_C);
	GPIO_port_enable(GPIO_port_D);

	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 6), GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
	
	// GPIO D3 analog in - PD3 - A4 - ADC1
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 3), GPIO_pinMode_I_analog, GPIO_Speed_In);

	// GPIO D4 analog in - PD4 - A7 - ADC2
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 4), GPIO_pinMode_I_analog, GPIO_Speed_In);

	// GPIO C4 analog in - PC4 - A2 - ADC3
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_C, 4), GPIO_pinMode_I_analog, GPIO_Speed_In);

	// GPIO D2 analog in - PD2 - A3 - ADC4
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 2), GPIO_pinMode_I_analog, GPIO_Speed_In);
	
	GPIO_ADC_set_sampletimes_all(GPIO_ADC_sampletime);
	GPIO_ADCinit();

	printf("ADC test\n");
	while(1)
	{
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), high);
		Delay_Ms( DELAYS );
		GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);
		Delay_Ms( DELAYS );
		// Read all 4 adc
		adc1 = GPIO_analogRead(GPIO_Ain4_D3);
		adc2 = GPIO_analogRead(GPIO_Ain7_D4);
		adc3 = GPIO_analogRead(GPIO_Ain2_C4);
		adc4 = GPIO_analogRead(GPIO_Ain3_D2);
		printf("ADC: %d | %d | %d | %d\n", adc1, adc2, adc3, adc4);
	}
}
