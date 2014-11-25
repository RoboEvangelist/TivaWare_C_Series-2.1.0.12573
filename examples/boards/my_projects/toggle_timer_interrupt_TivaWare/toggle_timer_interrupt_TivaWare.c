#include <stdint.h>
#include <stdbool.h>
#include "toggle_timer_interrupt_TivaWare.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "inc/tm4c1294ncpdt.h"

//*****************************************************************************
//
//!
//! A very simple example that uses a general purpose timer generated periodic 
//! interrupt to toggle the on-board LED D1 (PN1).
//
//*****************************************************************************

void
PortFunctionInit(void)
{
    //
    // Enable Peripheral Clocks 
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //
    // Enable pin PN1 for GPIOOutput
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
}

void Timer0A_Init(unsigned long period)
{   
	//
  // Enable Peripheral Clocks 
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC); 		// configure for 32-bit timer mode
  TimerLoadSet(TIMER0_BASE, TIMER_A, period -1);      //reload value
	IntPrioritySet(INT_TIMER0A, 0x00);  	 							// configure Timer0A interrupt priority as 0
  IntEnable(INT_TIMER0A);    													// enable interrupt 19 in NVIC (Timer0A)
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // arm timeout interrupt
  TimerEnable(TIMER0_BASE, TIMER_A);      						// enable timer0A
}

//interrupt handler for Timer0A
void Timer0A_Handler(void)
{
		// acknowledge flag for Timer0A timeout
		TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	
		// Toggle the LED D1 (PN1).
    GPIO_PORTN_DATA_R ^=0x02;

}

int main(void)
{	
		unsigned long period = 8000000; //reload value to Timer0A to generate half second delay
	
		//initialize the GPIO ports	
		PortFunctionInit();
	
    // Turn on the LED D1 (PN1).
    GPIO_PORTN_DATA_R |= 0x02;

    //initialize Timer0A and configure the interrupt
		Timer0A_Init(period);
	
		IntMasterEnable();        		// globally enable interrupt
	
    //
    // Loop forever.
    //
    while(1)
    {

    }
}
