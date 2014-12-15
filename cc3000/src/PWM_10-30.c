//*****************************************************************************
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "pwm.h"
#include "PWM_10-30.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom_map.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/tm4c1294ncpdt.h"
#include	"driverlib/uart.h"
#include	"driverlib/timer.h"
#include	"driverlib/interrupt.h"
//#define PWM_FREQUENCY	1000|
//#define APP_PI 3.1415926535897932384626433832795f
//#define STEPS 256
volatile uint8_t count;
//*****************************************************************************
void
PortFunctionInit(void)
{
    //
    // Enable Peripheral Clocks 
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

    //
    // Enable pin PG0 for PWM0 M0PWM4
    //
    MAP_GPIOPinConfigure(GPIO_PG0_M0PWM4);
    MAP_GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

    //
    // Enable pin PG1 for PWM0 M0PWM5
    //
    MAP_GPIOPinConfigure(GPIO_PG1_M0PWM5);
    MAP_GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_1);

    //
    // Enable pin PF3 for PWM0 M0PWM3
    //
    MAP_GPIOPinConfigure(GPIO_PF3_M0PWM3);
    MAP_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_3);

    //
    // Enable pin PF2 for PWM0 M0PWM2
    //
    MAP_GPIOPinConfigure(GPIO_PF2_M0PWM2);
    MAP_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
    
    //
    // Enable pin PA7 for GPIOOutput
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTA_AHB_BASE, GPIO_PIN_7);
    GPIO_PORTA_AHB_PUR_R |= 0x07; 
		//
		//Enable pin pk4 as pwm GPIOoutput
		//
		MAP_GPIOPinConfigure(GPIO_PK4_M0PWM6);
    MAP_GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_4);
}



void PWMINIT(void)
	//intializes PWM module 0 genrator 1,2 bits 4,5,2,3 PG0,PG1,PF2,PF3 respectively at 40 Mhz clock rate configured PWM clock for 500KHz
{
		//int ui32SysClkFreq;
		//ui32SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 40000000); // configure the system clock to be 40MHz
		PortFunctionInit();
		PWMGenConfigure(PWM0_BASE, PWM_GEN_2|PWM_GEN_3, PWM_GEN_MODE_DOWN);
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2|PWM_GEN_3,8000);
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3,800);
		//for 5 Khz 1/5KHz which is 200 micro seconds for 40MHz clock cycle it is 8000 cycles
		PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN);
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1,8000);
		PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
		PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, true);
		PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
		PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
		PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT, true);
		PWMGenEnable(PWM0_BASE, PWM_GEN_2|PWM_GEN_3); //PK4
		PWMGenEnable(PWM0_BASE, PWM_GEN_1);
	//PWMClockSet( PWM0_BASE,PWM_SYSCLK_DIV_1);     //set the PWM clk to 31250 HZ
	
	//PWMClockGet(PWM0_BASE);
}
void Forward1(void)
{
	//UARTprintf(" F1 checked\n");
	//PWMPulseWidthSet (PWM0_BASE, PWM_OUT_4, 4000);	// set the pwm duty cycle to .%
	//PWMPulseWidthSet (PWM0_BASE, PWM_OUT_5, 400);	// set the PWM duty cycle to 50%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_3, 2000);	// set the pwm duty cycle to 25%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_2, 200);	// set the PWM duty cycle to 2.5%
}
void Forward2(void)
{
	//UARTprintf(" F2 checked\n");
	//PWMPulseWidthSet (PWM0_BASE, PWM_OUT_4, 4000);	// set the pwm duty cycle to .%
	//PWMPulseWidthSet (PWM0_BASE, PWM_OUT_5, 400);	// set the PWM duty cycle to 50%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_3, 3000);	// set the pwm duty cycle to 37.5%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_2, 300);	// set the PWM duty cycle to 3.75%
}
void Backward2(void)
{
	//UARTprintf(" B2 checked\n");
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_3, 300);	// set the pwm duty cycle to 3.75%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_2, 3000);	// set the PWM duty cycle to 37.5%
}
void Backward1(void)
{
	//UARTprintf(" B1 checked\n");
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_3, 200);	// set the pwm duty cycle to 2.5%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_2, 2000);	// set the PWM duty cycle to 25%
}
void STOP(void)
{
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_4, 4000);	// set the pwm duty cycle to 50%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_5, 4000);	// set the PWM duty cycle to 50%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_3, 4000);	// set the pwm duty cycle to 50%
	PWMPulseWidthSet (PWM0_BASE, PWM_OUT_2, 4000);	// set the PWM duty cycle to 50%
}
void LForward1(void)
{
	//UARTprintf(" LF1 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 2000);			//25% duty cycle  PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 200);			//2.5% duty cycle PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 4000);			//50% duty cycle  PG 0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 400);			//5% duty cycle	  PG 1
}
void LForward2(void)
{
	//UARTprintf(" LF1 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 3000);			//37.5% duty cycle PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 300);			//3.75% duty cycle PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 4000);			//50% duty cycle PG 0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 400);			//5% duty cycle	PG 1
}
void LBackward1(void)
{
	//UARTprintf(" LB1 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 200);			//2.5% duty cycle PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 2000);			//25% duty cycle PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 4000);			//50% duty cycle PG 0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 400);			//5% duty cycle	PG 1
}
void LBackward2(void)
{
	//UARTprintf(" LB2 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 200);			//2.5% duty cycle PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 2000);			//25% duty cycle PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 4000);			//50% duty cycle PG 0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 400);			//5% duty cycle PG 1
}
void RForward1(void)
{
	//UARTprintf(" RF1 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 2000);			//100% duty cycle PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 200);				//10% duty cycle	PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 400);			//100% duty cycle PF 1
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 4000);				//10% duty cycle	PF 0
}
void RForward2(void)
{
	//UARTprintf(" RF2 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 3000);			//37.5% duty cycle PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 300);			//3.75% duty cycle PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 400);			//5% duty cycle PG 0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 4000);			//50% duty cycle PG 1
}
void RBackward1(void)
{
	//UARTprintf(" RB1 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 200);			//2.5% duty cycle PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 2000);			//25% duty cycle PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 400);			//5% duty cycle PG 0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 4000);			//50% duty cycle PG 1
}
void RBackward2(void)
{
	//UARTprintf(" RB2 checked\n");
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 200);			//100% duty cycle PF 2
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 2000);				//10% duty cycle	PF 3
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 400);			//100% duty cycle PF 1
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 4000);				//10% duty cycle	PF 0
}

void servotest1(void)
	//pulse width is 50% which keeps the motor in postion 0(0degree)
{
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 40);	//50% duty cycle PF0 PK4
}
void servotest2(void)
	//pulse width is 50% which keeps the motor in postion 1(90degree)
{
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 60);	//50% duty cycle PF0 PK4
}

void servotest3(void)
	//pulse width is 50% which keeps the motor in postion 1(90degree)
{
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 80);	//50% duty cycle PF0 PK4
}

void Reset(void)
{
	GPIOPinWrite(GPIO_PORTA_AHB_BASE, GPIO_PIN_7, 0x00);	 	//intialize prta 7 to reset entire system 
}
