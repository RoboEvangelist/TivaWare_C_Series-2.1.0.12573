#include "PWM_10-30.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/systick.h"
#include "driverlib/fpu.h"
#include "driverlib/debug.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "utils/cmdline.h"
#include "driverlib/uart.h"
#include "driverlib/ssi.h"
#include "inc/hw_types.h"
#include	"driverlib/fpu.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/tm4c1294ncpdt.h"     // registers mapping file
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"


// variables for the left and right IR sensors
uint32_t ui32IRValues[2];              // each value represents a sensor data
volatile uint32_t ui32LeftSensor;      // PE3
volatile uint32_t ui32RightSensor;     // PE4
//volatile uint32_t ui32FrontSensor;     // PE5
volatile uint32_t ui32SensorsDiff;     // difference between Left and Right sensor

// minimum obstacle avoidance threshold in cm
volatile const uint8_t ui8ObstacleDistance = 50;
// time delay
volatile const uint32_t ui32TimeDelay = 1000000;

// use these booleans to avoid sensing the same command over and over
bool boolMovingForward = false;
bool boolTurningLeft = false;
bool boolTurningRight = false;
bool boolMovingBack = false;
bool boolStopped = false;

void ADC0_InitSWTriggerSeq3_Ch9(void){ 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);   // activate the clock of ADC0
	while((SYSCTL_PRADC_R&SYSCTL_PRADC_R0) == 0){};
		
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  // activate the clock of E
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){};
	
	// Enable PE3 PE4 PE5 as analog	
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 );
		
	ADCSequenceDisable(ADC0_BASE, 1); //disable ADC0 before the configuration is complete
		
	// use ADC0, SS1 (4 samples max), processor trigger, priority 0
	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
		
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH0); // PE3/analog Input 0 - Left sensor
	//ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH9|ADC_CTL_IE|ADC_CTL_END); // PE4/analog Input 9  - Right sensor
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH8|ADC_CTL_IE|ADC_CTL_END);  // PE5/analog Input 8 - Middle sensor
	//ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_CH8|ADC_CTL_IE|ADC_CTL_END);  // P	`E5/analog Input 8 - Middle sensor
	ADCSequenceEnable(ADC0_BASE, 1);
}

void ADC0_InSeq3(void){  
	ADCIntClear(ADC0_BASE, 1);
	ADCProcessorTrigger(ADC0_BASE, 1);
	while(!ADCIntStatus(ADC0_BASE, 1, false))
	{
	}
	ADCSequenceDataGet(ADC0_BASE, 1, ui32IRValues);   // get data from FIFO
	
	// resutl is given in voltage, and the formula gives (1/cm)
	// so we invert the result of the convertion to get (cm)
	//ui32LeftSensor = 1.0/(powf(7.0, -5)*ui32IRValues[0]*1.0 - 0.0022);
	ui32LeftSensor = 1.0/((37.0/648000)*ui32IRValues[0]*1.0-(67.0/6480.0));
	//ui32RightSensor = 1.0/(powf(7.0, -5)*ui32IRValues[1]*1.0 - 0.0022);
	ui32RightSensor = 1.0/((37.0/648000)*ui32IRValues[1]*1.0-(67.0/6480.0));
	
	// get difference in reading between motors only when an object is close enough
	if (ui32RightSensor <= 50 || ui32LeftSensor <= 50)  // if less than 60 cm
		ui32SensorsDiff = ui32RightSensor - ui32LeftSensor;
	else 
		ui32SensorsDiff = 0;
	// resutl is given in voltage, and the formula gives (1/cm)
	// so we invert the result of the convertion to get (cm)
	
}

//interrupt handler
void ADC0_Handler(void)
{
		ADCIntClear(ADC0_BASE, 1);
		ADCProcessorTrigger(ADC0_BASE, 1);
		ADCSequenceDataGet(ADC0_BASE, 1, ui32IRValues);
}