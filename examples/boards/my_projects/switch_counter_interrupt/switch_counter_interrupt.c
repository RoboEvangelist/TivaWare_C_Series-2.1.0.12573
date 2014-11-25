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
	
		// Enable the clock of the GPIO port J that is used for the on-board switch.
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R8;

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
Interrupt_Init(void)
{
  NVIC_EN1_R |= 0x00080000;  		// enable interrupt 51 in NVIC (GPIOJ)
	NVIC_PRI12_R &= ~0xE0000000; 		// configure GPIOJ interrupt priority as 0
	GPIO_PORTJ_AHB_IM_R |= 0x03;   		// arm interrupt on PJ0, PJ1
	GPIO_PORTJ_AHB_IS_R &= ~0x03;     // PJ0, PJ1 are edge-sensitive
  GPIO_PORTJ_AHB_IBE_R &= ~0x03;   	// PJ0, PJ1 not both edges trigger 
  GPIO_PORTJ_AHB_IEV_R &= ~0x03;  // PJ0, PJ1 falling edge event
	IntGlobalEnable();        		// globally enable interrupt
}

//interrupt handler
void GPIOPortJ_Handler(void)
{
	
	//SW1 is pressed
	if(GPIO_PORTJ_AHB_RIS_R&0x01)
	{
		// acknowledge flag for PJ0
		GPIO_PORTJ_AHB_ICR_R |= 0x01; 
		//counter imcremented by 1
		count++;
	}
	
	//SW2 is pressed
  if(GPIO_PORTJ_AHB_RIS_R&0x02)
	{
		// acknowledge flag for PJ1
		GPIO_PORTJ_AHB_ICR_R |= 0x02; 
		//counter imcremented by 1
		count++;
	}
	
	
}

int main(void)
{
	
		//initialize the GPIO ports	
		PortFunctionInit();
		
	//configure the GPIOF interrupt
		Interrupt_Init();
	
    //
    // Loop forever.
    //
    while(1)
    {

    }
}
