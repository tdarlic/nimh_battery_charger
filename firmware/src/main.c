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

#define DELAYS 100
#define MINLED 2
#define MAXLED 2000
#define LEDSPEED 200

#define LED_MAIN_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 2), low)
#define LED_MAIN_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 2), high)

#define LED1_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 7), low)
#define LED1_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 7), high)

#define LED2_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 1), low)
#define LED2_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_A, 1), high)

#define LED3_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 0), low)
#define LED3_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_D, 0), high)

#define LED4_ON GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_C, 0), low)
#define LED4_OFF GPIO_digitalWrite(GPIOv_from_PORT_PIN(GPIO_port_C, 0), high)

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
		GPIO_tim2_analogWrite(2, getpwmval());
	 }
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
		// printf("TIM1_CNT / TIM1_RPTCNT: %d / %d\r\n", TIM1->CNT, TIM1->RPTCR);
		// printf("[%d] ADC: %d | %d | %d | %d\n", millis(), adc1, adc2, adc3, adc4);
		(adc1 < 500) ? (LED1_ON) : (LED1_OFF);
		(adc2 < 500) ? (LED2_ON) : (LED2_OFF);
		(adc3 < 500) ? (LED3_ON) : (LED3_OFF);
		(adc4 < 500) ? (LED4_ON) : (LED4_OFF);
		Delay_Ms( DELAYS );
	}
}
