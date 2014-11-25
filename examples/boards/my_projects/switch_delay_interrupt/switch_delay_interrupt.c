#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/sysctl.h"


#define 	BLUE_MASK 		0x04
//*****************************************************************************
//
//!
//! A very simple example that interfaces with the LED D1 (PN1) and SW1 (PJ0) 
//! using direct register access. When SW2 is pressed, the LED is turned on. When 
//! SW1 is released, the LED is turned off. Interrupt on PF0 is configured as 
//! edge-triggered on both edges
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

//Globally enable interrupts 
void IntGlobalEnable(void)
{
    __asm("    cpsie   i\n");
}

void
Interrupt_Init(void)
{
  NVIC_EN1_R |= 0x00080000;  		// enable interrupt 51 in NVIC (GPIOJ)
	NVIC_PRI12_R &= ~0xE0000000; 		// configure GPIOJ interrupt priority as 0
	GPIO_PORTJ_AHB_IM_R |= 0x01;   		// arm interrupt on PJ0
	GPIO_PORTJ_AHB_IS_R &= ~0x01;     // PJ0 is edge-sensitive
  GPIO_PORTJ_AHB_IBE_R |= 0x01;   	// PJ0 both edges trigger 
  //GPIO_PORTJ_AHB_IEV_R &= ~0x01;  // PJ0 falling edge event
	IntGlobalEnable();        		// globally enable interrupt
}

//interrupt handler
void GPIOPortJ_Handler(void){
	
	// acknowledge flag for PJ0
  GPIO_PORTJ_AHB_ICR_R |= 0x01;      
	
	//SW1 is pressed
  if((GPIO_PORTJ_AHB_DATA_R&0x01)==0x00) 
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

      // Delay for a bit.
				SysCtlDelay(6000000);	
    }
}
