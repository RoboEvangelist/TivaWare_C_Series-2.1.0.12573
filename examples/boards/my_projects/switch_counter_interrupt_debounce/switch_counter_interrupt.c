#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/sysctl.h"

//*****************************************************************************
//
//!
//! Design a counter. The counter is incremented by 1 when SW1 (PJ0) or SW2 (PJ1) 
//! is pressed.
//
//*****************************************************************************

// global variable visible in Watch window of debugger
// increments at least once per button press
volatile unsigned long count = 0;

void
PortFunctionInit(void)
{

		volatile uint32_t ui32Loop;   
	
		// Enable the clock of the GPIO port N that is used for the on-board LED D1 and GPIO Port J that is used for SW1.
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R12|SYSCTL_RCGCGPIO_R8;

    //
    // Do a dummy read to insert a few cycles after enabling the peripheral.
    //
    ui32Loop = SYSCTL_RCGCGPIO_R;
	
		// Set the direction of PJ0 (SW1) and PJ1 (SW2) as input by clearing the bit
    GPIO_PORTJ_AHB_DIR_R &= ~0x03;
	
    // Enable PJ0, and PJ1 for digital function.
    GPIO_PORTJ_AHB_DEN_R |= 0x03;
	
		//Enable pull-up on PJ0 and PJ1
		GPIO_PORTJ_AHB_PUR_R |= 0x03;

		// Set the direction of PN1 (LED D1) and PN0 (LED D0) as output
    GPIO_PORTN_DIR_R |= 0x03;
		// Enable PN1 for digital function.
    GPIO_PORTN_DEN_R |= 0x03;
}

//Globally enable interrupts 
void IntGlobalEnable(void)
{
    __asm("    cpsie   i\n");
}

//Globally disable interrupts 
void IntGlobalDisable(void)
{
    __asm("    cpsid   i\n");
}

void
Button_Interrupt_Init(void)
{
  NVIC_EN1_R |= 0x00080000;  		    // enable interrupt 51 in NVIC (GPIOJ)
	NVIC_PRI12_R &= ~0x40000000; 		  // configure GPIOJ interrupt priority as 2
	GPIO_PORTJ_AHB_IM_R |= 0x03;   		// arm interrupt on PJ0, PJ1
	GPIO_PORTJ_AHB_IS_R &= ~0x03;     // PJ0, PJ1 are edge-sensitive
  GPIO_PORTJ_AHB_IBE_R |= 0x03;   	// PJ0, PJ1  both edges trigger 
  //GPIO_PORTJ_AHB_IEV_R &= ~0x03;  // PJ0, PJ1 falling edge event
}

//interrupt handler
void GPIOPortJ_Handler(void)
{
	//switch debounce
	NVIC_EN1_R &= ~0x00080000;  // disable interrupt 51 in NVIC (GPIOJ)
	SysCtlDelay(53333);	        // Delay for a while
	NVIC_EN1_R |= 0x00080000;   // enable interrupt 51 in NVIC (GPIOJ)

	//SW1 has action
	if(GPIO_PORTJ_AHB_RIS_R&0x01)
	{
		// acknowledge flag for PJ0
		GPIO_PORTJ_AHB_ICR_R |= 0x01; 
		
		//SW1 is pressed
		if((GPIO_PORTJ_AHB_DATA_R&0x01)==0x00) 
		{
			//counter imcremented by 1
			count++;
			// Turn on the LED1.
			GPIO_PORTN_DATA_R |= 0x02;
		}
		else
		{
			// Turn off the LED.
			GPIO_PORTN_DATA_R &= ~0x02;
		}
	}
	
	//SW2 has action
  if(GPIO_PORTJ_AHB_RIS_R&0x02)
	{
		// acknowledge flag for PJ1
		GPIO_PORTJ_AHB_ICR_R |= 0x02; 
		
		//SW2 is pressed
		if((GPIO_PORTJ_AHB_DATA_R&0x02)==0x00) 
		{
			//counter imcremented by 1
			count++;
			// Turn on the LED0.
			GPIO_PORTN_DATA_R |= 0x01;
		}
		else
		{
			// Turn off the LED0.
			GPIO_PORTN_DATA_R &= ~0x01;
		}
	}
}

void Timer0A_Init(unsigned long period)
{   
	volatile uint32_t ui32Loop; 
	
	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0; // activate timer0
  ui32Loop = SYSCTL_RCGCTIMER_R;				// Do a dummy read to insert a few cycles after enabling the peripheral.
  TIMER0_CTL_R &= ~0x00000001;     // disable timer0A during setup
  TIMER0_CFG_R = 0x00000000;       // configure for 32-bit timer mode
  TIMER0_TAMR_R = 0x00000002;      // configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = period-1;       // reload value
	NVIC_PRI4_R &= ~0xE0000000; 	 // configure Timer0A interrupt priority as 0
  NVIC_EN0_R |= 0x00080000;     // enable interrupt 19 in NVIC (Timer0A)
	TIMER0_IMR_R |= 0x00000001;      // arm timeout interrupt
  TIMER0_CTL_R |= 0x00000001;      // enable timer0A
}

//interrupt handler for Timer0A
void Timer0A_Handler(void)
{
		// acknowledge flag for Timer0A
		TIMER0_ICR_R |= 0x00000001; 
	
		// Toggle the LED.
    GPIO_PORTN_DATA_R ^=0x03;
}

int main(void)
{	
		unsigned long period = 8000000; //reload value to Timer0A to generate half second delay

		//initialize the GPIO ports	
		PortFunctionInit();
	
		// Turn on the LED.
    GPIO_PORTN_DATA_R |= 0x02;
		
	//initialize Timer0A and configure the interrupt
		Timer0A_Init(period);
	//configure the GPIOJ interrupt
		//Button_Interrupt_Init();
	
	  IntGlobalEnable();        		    // globally enable interrupt
	
    //
    // Loop forever.
    //
    while(1)
    {
    }
}
