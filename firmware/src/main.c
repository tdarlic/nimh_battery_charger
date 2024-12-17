#include "ch32v003_GPIO_branchless.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

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


// Holds next transition for flasing / PWM leds
uint32_t next = 0;

struct adcval {
		uint16_t adc;
		uint16_t padc;
		uint32_t timer;
};

// Holds the last value of ADC
volatile uint16_t adc1 = 2000, adc2 = 2000, adc3 = 2000, adc4 = 2000;
// Holds the previous value of ADC
volatile uint16_t ch1 = 0, ch2 = 0, ch3 = 0, ch4 = 0;
// vaues of timer
uint32_t tm1 = 0, tm2 = 0, tm3 = 0, tm4 = 0;
// boolean values for PWM
bool cp1 = true, cp2 = true, cp3 = true, cp4 = true;
 
#define DELAYS 200
// ADC treshold - any value below this the system will presume that the battery is charging
#define ADCTR 500

// LED_PERIOD defines PWM duration period
#define LED_PERIOD 500

// After CHARGING_PERIOD in second the LED will not blink indicating charged battery
// By default this is 24 hours
#define CHARGING_PERIOD 24 * 60 * 60

#define BAT1_IN (adc1 < ADCTR) 
#define BAT2_IN (adc2 < ADCTR) 
#define BAT3_IN (adc3 < ADCTR) 
#define BAT4_IN (adc4 < ADCTR) 

#define BAT1_PLUGGED (ch1 > ADCTR)
#define BAT2_PLUGGED (ch2 > ADCTR)
#define BAT3_PLUGGED (ch3 > ADCTR)
#define BAT4_PLUGGED (ch4 > ADCTR)

#define LED_MAIN_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 2), low);
#define LED_MAIN_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 2), high)

#define LED1_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 7), low)
#define LED1_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 7), high)

#define LED2_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 1), low)
#define LED2_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 1), high)

#define LED3_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 0), low)
#define LED3_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 0), high)

#define LED4_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_C, 0), low)
#define LED4_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_C, 0), high)

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

	// Enable the SysTick IRQ
	NVIC_EnableIRQ(SysTicK_IRQn);
}

void pwm(void)
{
	// This holds the length of the PWM cycle in ms
	const uint16_t length = LED_PERIOD; 
	static bool led_on = true;
	if (systick_millis > next) { 
		if (led_on){
			(BAT1_IN) ? (LED1_ON) : (LED1_OFF);
			(BAT2_IN) ? (LED2_ON) : (LED2_OFF);
			(BAT3_IN) ? (LED3_ON) : (LED3_OFF);
			(BAT4_IN) ? (LED4_ON) : (LED4_OFF);
			led_on = false;
		} else {
			if (BAT1_IN && (cp1)) {LED1_OFF;}
			if (BAT2_IN && (cp2)) {LED2_OFF;}
			if (BAT3_IN && (cp3)) {LED3_OFF;}
			if (BAT4_IN && (cp4)) {LED4_OFF;}
			led_on = true;
		}
		next = systick_millis + LED_PERIOD;
	}
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

	// do this every second
	if ((systick_millis % 1000) == 0){
		if (BAT1_IN) {tm1++;}
		if (BAT2_IN) {tm2++;}
		if (BAT3_IN) {tm3++;}
		if (BAT4_IN) {tm4++;}
	}
	pwm();
}


int FLASH_GetBank1Status(void)
{
	int flashstatus = FLASH_COMPLETE;

	if((FLASH->STATR & FLASH_FLAG_BANK1_BSY) == FLASH_FLAG_BSY)
	{
		flashstatus = FLASH_BUSY;
	}
	else
{
		if((FLASH->STATR & FLASH_FLAG_BANK1_WRPRTERR) != 0)
		{
			flashstatus = FLASH_ERROR_WRP;
		}
		else
	{
			flashstatus = FLASH_COMPLETE;
		}
	}
	return flashstatus;
}

int FLASH_WaitForLastOperation(uint32_t Timeout)
{
	int status = FLASH_COMPLETE;

	status = FLASH_GetBank1Status();
	while((status == FLASH_BUSY) && (Timeout != 0x00))
	{
		status = FLASH_GetBank1Status();
		Timeout--;
	}
	if(Timeout == 0x00)
	{
		status = FLASH_TIMEOUT;
	}
	return status;
}

int main()
{
	SystemInit();
	systick_init();
	int status = 0;
	uint16_t OB_STOP = OB_STOP_NoRST;
	uint16_t OB_IWDG = OB_IWDG_SW;
	uint16_t OB_STDBY = OB_STDBY_NoRST;
	uint16_t OB_RST = OB_RST_NoEN;
	uint16_t OB_BOOT = OB_STARTMODE_BOOT;
	struct adcval adcl[4];

	FLASH->OBKEYR = FLASH_KEY1;
	FLASH->OBKEYR = FLASH_KEY2;
	status = FLASH_WaitForLastOperation(10000);

	if(status == FLASH_COMPLETE)
	{
		FLASH->CTLR |= CR_OPTPG_Set;
		OB->USER = OB_BOOT | OB_IWDG | (uint16_t)(OB_STOP | (uint16_t)(OB_STDBY | (uint16_t)(OB_RST | (uint16_t)0xc0)));

		status = FLASH_WaitForLastOperation(10000);
		if(status != FLASH_TIMEOUT)
		{
			FLASH->CTLR &= CR_OPTPG_Reset;
		}
	}

	printf( "After Write:%04x\n", OB->USER );

	GPIO_tim1_init();
	NVIC_EnableIRQ(TIM1_UP_IRQn);

	GPIO_port_enable(GPIO_port_C);
	GPIO_port_enable(GPIO_port_D);
	GPIO_port_enable(GPIO_port_A);

	// MAIN LED
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A, 2), GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
	// LED1
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 7), GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
	// LED2
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A, 1), GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
	// LED3
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_D, 0), GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
	// LED4
	GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_C, 0), GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
	
	//  switch off all leds
	LED1_OFF;
	LED2_OFF;
	LED3_OFF;
	LED4_OFF;

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
		LED_MAIN_ON;
		// Read all 4 adc
		adc1 = GPIO_analogRead(GPIO_Ain4_D3);
		adc2 = GPIO_analogRead(GPIO_Ain7_D4);
		adc3 = GPIO_analogRead(GPIO_Ain2_C4);
		adc4 = GPIO_analogRead(GPIO_Ain3_D2);

		// Now reset the timer if battery has just been plugged
		if (BAT1_PLUGGED) {tm1 = 0; cp1 = true;}
		if (BAT2_PLUGGED) {tm2 = 0; cp2 = true;}
		if (BAT3_PLUGGED) {tm3 = 0; cp3 = true;}
		if (BAT4_PLUGGED) {tm4 = 0; cp4 = true;}

		// remeber the old value of ADC
		ch1 = adc1;
		ch2 = adc2;
		ch3 = adc3;
		ch4 = adc4;

		if (tm1 > CHARGING_PERIOD) {cp1 = false;}
		if (tm2 > CHARGING_PERIOD) {cp2 = false;}
		if (tm3 > CHARGING_PERIOD) {cp3 = false;}
		if (tm4 > CHARGING_PERIOD) {cp4 = false;}


		// printf("TIM1_CNT / TIM1_RPTCNT: %d / %d\r\n", TIM1->CNT, TIM1->RPTCR);
		printf("[%10d] ADC: %d | %d | %d | %d\n", millis(), adc1, adc2, adc3, adc4);
		printf("[%10d] TIM: %d | %d | %d | %d\n", millis(), tm1, tm2, tm3, tm4);
		printf("Next:%d\n", next);
		Delay_Ms( DELAYS );
	}
}
