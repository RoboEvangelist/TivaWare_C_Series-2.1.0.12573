#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "inc/tm4c1294ncpdt.h"

//*****************************************************************************
//
//!
//! In this project we use ADC0, SS1 to measure the data from the on-chip 
//! temperature sensor. The ADC sampling is triggered by software whenever 
//! four samples have been collected. Both the Celsius and the Fahrenheit  
//! temperatures are calcuated.
//
//*****************************************************************************

uint32_t ui32ADC0Value[4];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;

//ADC0 initializaiton
void ADC0_Init(void)
{
	
		int ui32SysClkFreq;
		ui32SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 40000000); // configure the system clock to be 40MHz
		SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);	//activate the clock of ADC0
		SysCtlDelay(2);	//insert a few cycles after enabling the peripheral to allow the clock to be fully activated.

		ADCSequenceDisable(ADC0_BASE, 1); //disable ADC0 before the configuration is complete
		ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // will use ADC0, SS1, processor-trigger, priority 0
		ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS); //ADC0 SS1 Step 0, sample from internal temperature sensor
		ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS); //ADC0 SS1 Step 1, sample from internal temperature sensor
		ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS); //ADC0 SS1 Step 2, sample from internal temperature sensor
		//ADC0 SS1 Step 0, sample from internal temperature sensor, completion of this step will set RIS, last sample of the sequence
		ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END); 
	
		IntPrioritySet(INT_ADC0SS1, 0x00);  	 // configure ADC0 SS1 interrupt priority as 0
		IntEnable(INT_ADC0SS1);    				// enable interrupt 31 in NVIC (ADC0 SS1)
		ADCIntEnableEx(ADC0_BASE, ADC_INT_SS1);      // arm interrupt of ADC0 SS1
	
		ADCSequenceEnable(ADC0_BASE, 1); //enable ADC0
}
		
//interrupt handler
void ADC0_Handler(void)
{
	
		ADCIntClear(ADC0_BASE, 1);
		ADCProcessorTrigger(ADC0_BASE, 1);
		ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
		ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
		ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
		ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;
		// Toggle the LED by doing circular addition (e.g., 0, 1, 2, 3, 0, 1...)
		GPIO_PORTN_DATA_R = (++GPIO_PORTN_DATA_R)%4;
}

int main(void)
{
		// initiate ports
		volatile uint32_t ui32Loop;
		// Enable the clock of the GPIO port N that is used for the on-board LED D1 and GPIO Port J that is used for SW1.
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R12;
    ui32Loop = SYSCTL_RCGCGPIO_R;

		// Set the direction of PN1 (LED D1) and PN0 (LED D0) as output
    GPIO_PORTN_DIR_R |= 0x03;
		// Enable PN1 and PN0 for digital function.
    GPIO_PORTN_DEN_R |= 0x03;
		
		// Turn on the LED.
    GPIO_PORTN_DATA_R |= 0x03;
		ADC0_Init();
		IntMasterEnable();       		// globally enable interrupt
		ADCProcessorTrigger(ADC0_BASE, 1);
	
		while(1)
		{
			
		}
}
