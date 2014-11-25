#include <stdint.h>
#include <stdbool.h>
#include "switch_counter_interrupt_TivaWare.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "inc/tm4c1294ncpdt.h"



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
    //
    // Enable Peripheral Clocks 
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    //
    // Enable pin PJ0 for GPIOInput
    //
    GPIOPinTypeGPIOInput(GPIO_PORTJ_AHB_BASE, GPIO_PIN_0);

    //
    // Enable pin PJ1 for GPIOInput
    //
    GPIOPinTypeGPIOInput(GPIO_PORTJ_AHB_BASE, GPIO_PIN_1);
	
		//Enable pull-up on PJ0 and PJ1
		GPIO_PORTJ_AHB_PUR_R |= 0x03; 

}


void
Interrupt_Init(void)
{
  IntEnable(INT_GPIOJ);  							// enable interrupt 51 in NVIC (GPIOJ)
	IntPrioritySet(INT_GPIOJ, 0x00); 		// configure GPIOJ interrupt priority as 0
	GPIO_PORTJ_AHB_IM_R |= 0x03;   		// arm interrupt on PJ0, PJ1
	GPIO_PORTJ_AHB_IS_R &= ~0x03;     // PJ0, PJ1 are edge-sensitive
  GPIO_PORTJ_AHB_IBE_R &= ~0x03;   	// PJ0, PJ1 not both edges trigger 
  GPIO_PORTJ_AHB_IEV_R &= ~0x03;  // PJ0, PJ1 falling edge event
	IntMasterEnable();        		// globally enable interrupt
}

//interrupt handler
void GPIOPortJ_Handler(void)
{
	
	//SW1 is pressed
	if(GPIO_PORTJ_AHB_RIS_R&0x01)
	{
		// acknowledge flag for PJ0
		GPIOIntClear(GPIO_PORTJ_AHB_BASE, GPIO_PIN_0);     
		//counter imcremented by 1
		count++;
	}
	
	//SW2 is pressed
  if(GPIO_PORTJ_AHB_RIS_R&0x02)
	{
		// acknowledge flag for PJ1
		GPIOIntClear(GPIO_PORTJ_AHB_BASE, GPIO_PIN_1);     
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
