#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"

#define 	BLUE_MASK 		0x04
//*****************************************************************************
//
//!
//! A very simple example that interfaces with the LED D1 (PN1) and SW1 (PJ0) 
//! using direct register access. When SW2 is pressed, the LED is turned on. When 
//! SW1 is released, the LED is turned off. 
//
//*****************************************************************************


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


    // Set the direction of PN1 (LED D1) as output
    GPIO_PORTN_DIR_R |= 0x02;
	
		// Set the direction of PJ0 (SW1) as input by clearing the bit
    GPIO_PORTJ_AHB_DIR_R &= ~0x01;
	
    // Enable PN1 for digital function.
    GPIO_PORTN_DEN_R |= 0x02;
	
    // Enable PJ0 for digital function.
    GPIO_PORTJ_AHB_DEN_R |= 0x01;
		
		//Enable pull-up on PJ0
		GPIO_PORTJ_AHB_PUR_R |= 0x01; 

}


int main(void)
{
	
		//initialize the GPIO ports	
		PortFunctionInit();
	
    //
    // Loop forever.
    //
    while(1)
    {

        if((GPIO_PORTJ_AHB_DATA_R&0x01)==0x00) //SW1 is pressed
				{
						// Turn on the LED.
						GPIO_PORTN_DATA_R |= 0x02;
				}
				else
				{
						// Turn off the LED.
						GPIO_PORTN_DATA_R &= ~0x02;
				}
    }
}
