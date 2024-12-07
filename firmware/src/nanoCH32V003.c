#include "ch32v003_GPIO_branchless.h"
#include <stdint.h>
#include <stdio.h>

// Number of ticks elapsed per millisecond (48,000 when using 48MHz Clock)
#define SYSTICK_ONE_MILLISECOND ((uint32_t)FUNCONF_SYSTEM_CORE_CLOCK / 1000)
// Number of ticks elapsed per microsecond (48 when using 48MHz Clock)
#define SYSTICK_ONE_MICROSECOND ((uint32_t)FUNCONF_SYSTEM_CORE_CLOCK / 1000000)

// Simple macro functions to give a arduino-like functions to call
// millis() reads the incremented systick variable
// micros() reads the raw SysTick Count, and divides it by the number of 
// ticks per microsecond ( WARN: Wraps every 90 seconds!)
#define millis() (systick_millis)
#define micros() (SysTick->CNT / SYSTICK_ONE_MICROSECOND)

// Incremented in the SysTick IRQ - in this example once per millisecond
volatile uint32_t systick_millis;

// This is only for devboard
uint16_t adc1, adc2, adc3, adc4 = 0;

#define PIN_LED PD6

#define DELAYS 1000
#define MINLED 2
#define MAXLED 1500
#define LEDSPEED 70

#define LED_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), high);
#define LED_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 6), low);

/**
* Returns value for PWM
* First it increases th value of counter 
* And than it cubes it
* Starts decreasing in case that PWM value is larger than MAXLEDPWM
* Starts increasing in case that counter is less than MINLED
*/
inline static uint16_t getpwmval(void){
	static uint16_t cnt = MINLED;
	static int8_t dir = 1;
	uint32_t out = 0;
	
	cnt = cnt + dir;
	out = cnt * cnt * cnt;
	// printf("out: %d\n", out);

	if (out > MAXLED){
		dir = -1;	
	}
	if (cnt < MINLED){
		dir = 1;
	}
	return out;
}

/*
 * Initialises the SysTick to trigger an IRQ with auto-reload, using HCLK/1 as
 * its clock source
 */
void systick_init(void)
{
	// Reset any pre-existing configuration
	SysTick->CTLR = 0x0000;
	
	// Set the compare register to trigger once per millisecond
	SysTick->CMP = SYSTICK_ONE_MILLISECOND - 1;

	// Reset the Count Register, and the global millis counter to 0
	SysTick->CNT = 0x00000000;
	systick_millis = 0x00000000;
	
	// Set the SysTick Configuration
	// NOTE: By not setting SYSTICK_CTLR_STRE, we maintain compatibility with
	// busywait delay funtions used by ch32v003_fun.
	SysTick->CTLR |= SYSTICK_CTLR_STE   |  // Enable Counter
	                 SYSTICK_CTLR_STIE  |  // Enable Interrupts
	                 SYSTICK_CTLR_STCLK ;  // Set Clock Source to HCLK/1
	//
		
	
	// Enable the SysTick IRQ
	NVIC_EnableIRQ(SysTicK_IRQn);
}

/*
 * SysTick ISR - must be lightweight to prevent the CPU from bogging down.
 * Increments Compare Register and systick_millis when triggered (every 1ms)
 * NOTE: the `__attribute__((interrupt))` attribute is very important
 */
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
	// Increment the Compare Register for the next trigger
	// If more than this number of ticks elapse before the trigger is reset,
	// you may miss your next interrupt trigger
	// (Make sure the IQR is lightweight and CMP value is reasonable)
	SysTick->CMP += SYSTICK_ONE_MILLISECOND;

	// Clear the trigger state for the next IRQ
	SysTick->SR = 0x00000000;

	// Increment the milliseconds count
	systick_millis++;
	if (!(systick_millis % LEDSPEED)){
		GPIO_tim2_analogWrite(3, getpwmval());
	 }
}

void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
void TIM1_UP_IRQHandler(void)
{
	printf("Evo me\r\n");
}

void printRCC(void){
	uint32_t regtemp, trim;
	regtemp = RCC->CTLR;
	printf("Printing RCC - clock control register\r\n");
	printf("RCC->CTLR: 0x%08lX:\r\n", (unsigned long) regtemp);
	if (regtemp & RCC_PLLRDY) printf("RCC_PLLRDY \r\n");
	if (regtemp & RCC_PLLON)  printf("RCC_PLLON  \r\n");
	if (regtemp & RCC_CSSON)  printf("RCC_CSSON  \r\n");
	if (regtemp & RCC_HSEBYP) printf("RCC_HSEBYP \r\n");
	if (regtemp & RCC_HSERDY) printf("RCC_HSERDY \r\n");
	if (regtemp & RCC_HSEON)  printf("RCC_HSEON  \r\n");
	if (regtemp & RCC_HSIRDY) printf("RCC_HSIRDY \r\n");
	if (regtemp & RCC_HSION)  printf("RCC_HSION  \r\n");
	printf("\r\n");
	printf("HSICAL: 0x%02lX\r\n", (unsigned long) (regtemp & RCC_HSICAL) >> 8);
	trim = (regtemp & RCC_HSITRIM) >> 3;
	printf("HSITRIM: 0x%02lX\r\n", (unsigned long) trim);
}

void printTIM1(void){
	uint32_t ctrl1, ctrl2;
	ctrl1 = TIM1->CTLR1;
	ctrl2 = TIM1->INTFR;
	printf("Printing TIM1 registers:\r\n");
	printf("CTRL1: %X\r\n", ctrl1);
	printf("CTRL2: %X\r\n", ctrl1);
	printf("OPM: %d\r\n", (uint8_t) (ctrl1 & TIM_OPM));
	printf("DIR: %d\r\n", (uint8_t) (ctrl1 & TIM_DIR));
	printf("TIF: %d\r\n", (uint8_t) (ctrl1 & TIM_TIF));
}

int main()
{
	SystemInit();
	systick_init();

	GPIO_tim1_init();
	NVIC_EnableIRQ(TIM1_UP_IRQn);

	printRCC();
	printTIM1();

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

	GPIO_port_pinMode(GPIO_port_D, GPIO_pinMode_O_pushPullMux, GPIO_Speed_10MHz);
	GPIO_tim2_map(GPIO_tim2_output_set_3__C1_C7_D6_D5);
	GPIO_tim2_init();
	GPIO_tim2_enableCH(3);


	printf("ADC test\n");
	while(1)
	{
		// Read all 4 adc
		adc1 = GPIO_analogRead(GPIO_Ain4_D3);
		adc2 = GPIO_analogRead(GPIO_Ain7_D4);
		adc3 = GPIO_analogRead(GPIO_Ain2_C4);
		adc4 = GPIO_analogRead(GPIO_Ain3_D2);
		printf("TIM1_CNT / TIM1_RPTCNT: %d / %d\r\n", TIM1->CNT, TIM1->RPTCR);
		printf("[%d] ADC: %d | %d | %d | %d\n", millis(), adc1, adc2, adc3, adc4);
		Delay_Ms( DELAYS );
	}
}
