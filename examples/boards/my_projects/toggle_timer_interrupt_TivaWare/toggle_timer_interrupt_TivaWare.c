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
#include "driverlib/pwm.h"
volatile uint8_t count;
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
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

    //
    // Enable pin PN1 for GPIOOutput
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1|GPIO_PIN_0);
	
		//
		//Enable pin pk4 as pwm GPIOoutput
		//
		GPIOPinConfigure(GPIO_PK4_M0PWM6);
    GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_4);
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
void pwm(void)
{
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3,800);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_3);
	
}




//interrupt handler for Timer0A
void Timer0A_Handler(void)
{
		// acknowledge flag for Timer0A timeout
		TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	if(count>4)
	{
		count=0;
	}
	else{
				count++;
	}
	if(count==0)
				{
					PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 40);	//50% duty cycle PF0 PK4
					GPIO_PORTN_DATA_R= 0x03;
				}
				else if(count==1)
				{
					
					PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 60);	//50% duty cycle PF0 PK4
					GPIO_PORTN_DATA_R= 0x03;
				}
				else if(count==2)
				{
					PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 80);					//50% duty cycle PF0 PK4
					GPIO_PORTN_DATA_R= 0x03;
				}
				else
				{
					count=0;
					PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 40);	//50% duty cycle PF0 PK4
					GPIO_PORTN_DATA_R= 0x03;
					//LED D1,D2 off
				}

	

		// Toggle the LED D1 (PN1).
    GPIO_PORTN_DATA_R ^=0x02;

}

int main(void)
{	
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3,800);
		PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
		PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, true);
		PWMGenEnable(PWM0_BASE, PWM_GEN_3);

		unsigned long period = 32000000; //reload value to Timer0A to generate half second delay
		
		//initialize the GPIO ports	
		PortFunctionInit();
	
    // Turn on the LED D1 (PN1).
    GPIO_PORTN_DATA_R |= 0x03;

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
