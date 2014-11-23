#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/sysctl.h"

#define 	BLUE_MASK 		0x02
//*****************************************************************************
//
//!
//! A very simple example that toggles the on-board  LED D1 using direct register
//! access.
//
//*****************************************************************************


void
PortFunctionInit(void)
{
//
		volatile uint32_t ui32Loop;   
	// Enable the clock of the GPIO port N that is used for the on-board LED D1 
    //
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R12;

    //
    // Do a dummy read to insert a few cycles after enabling the peripheral.
    //
    ui32Loop = SYSCTL_RCGCGPIO_R;

    //
    // Enable the GPIO pin for the LED D1(PN1).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIO_PORTN_DIR_R |= 0x02;
    GPIO_PORTN_DEN_R |= 0x02;

}


int main(void)
{
	
		//initialize the GPIO ports	
		PortFunctionInit();
	
    // Turn on the LED.
    GPIO_PORTN_DATA_R |= 0x02;

    
    //
    // Loop forever.
    //
    while(1)
    {
        // Delay for a bit.
				SysCtlDelay(2000000);	

        // Toggle the LED.
        GPIO_PORTN_DATA_R ^=BLUE_MASK;
    }
}
