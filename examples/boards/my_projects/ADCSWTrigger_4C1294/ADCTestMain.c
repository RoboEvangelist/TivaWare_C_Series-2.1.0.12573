// ADCTestMain.c
// Runs on TM4C1294
// This program periodically samples ADC channel 0 and stores the
// result to a global variable that can be accessed with the JTAG
// debugger and viewed with the variable watch feature.
// Daniel Valvano
// April 8, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// center of X-ohm potentiometer connected to PE3/AIN0
// bottom of X-ohm potentiometer connected to ground
// top of X-ohm potentiometer connected to +3.3V
#include <stdint.h>
#include "ADCSWTrigger.h"
#include "tm4c1294ncpdt.h"
#include "PLL.h"

#define GPIO_PORTN1             (*((volatile uint32_t *)0x40064008))

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical(void);     // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

volatile uint32_t ADCvalue;
// This debug function initializes Timer0A to request interrupts
// at a 10 Hz frequency.  It is similar to FreqMeasure.c.
void Timer0A_Init10HzInt(void){
  DisableInterrupts();
  // **** general initialization ****
                                   // activate timer0
  SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;
                                   // allow time for clock to stabilize
  while((SYSCTL_PRTIMER_R&SYSCTL_PRTIMER_R0) == 0){};
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
  TIMER0_CC_R &= ~TIMER_CC_ALTCLK; // timer0 clocked from system clock
  // **** timer0A initialization ****
                                   // configure for periodic mode
  TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
  TIMER0_TAPR_R = 199;             // prescale value for 10us
  TIMER0_TAILR_R = 59999;          // start value for 10 Hz interrupts
  TIMER0_IMR_R |= TIMER_IMR_TATOIM;// enable timeout (rollover) interrupt
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// clear timer0A timeout flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, periodic, interrupts
  // **** interrupt initialization ****
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R = 0x00080000;         // enable interrupt 19 in NVIC
}
void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;    // acknowledge timer0A timeout
  GPIO_PORTN1 ^= 0x02;                  // profile
  ADCvalue = ADC0_InSeq3();             // takes 2.25 usec to run
//  GPIO_PORTN1 = 0x00;
}
int main(void){
  PLL_Init();                      // 120 MHz
                                   // activate clock for Port N
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;
                                   // allow time for clock to stabilize
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};
//  ADC0_InitSWTriggerSeq3_Ch0();    // initialize ADC0, software trigger, PE3/AIN0
  ADC0_InitSWTriggerSeq3(0);       // initialize ADC0, software trigger, PE3/AIN0
//  ADC0_InitSWTriggerSeq3(1);       // initialize ADC0, software trigger, PE2/AIN1
//  ADC0_InitSWTriggerSeq3(2);       // initialize ADC0, software trigger, PE1/AIN2
//  ADC0_InitSWTriggerSeq3(3);       // initialize ADC0, software trigger, PE0/AIN3
//  ADC0_InitSWTriggerSeq3(4);       // initialize ADC0, software trigger, PD7/AIN4
//  ADC0_InitSWTriggerSeq3(8);       // initialize ADC0, software trigger, PE5/AIN8
//  ADC0_InitSWTriggerSeq3(9);       // initialize ADC0, software trigger, PE4/AIN9
//  ADC0_InitSWTriggerSeq3(10);      // initialize ADC0, software trigger, PB4/AIN10
//  ADC0_InitSWTriggerSeq3(11);      // initialize ADC0, software trigger, PB5/AIN11
//  ADC0_InitSWTriggerSeq3(12);      // initialize ADC0, software trigger, PD3/AIN12
//  ADC0_InitSWTriggerSeq3(13);      // initialize ADC0, software trigger, PD2/AIN13
//  ADC0_InitSWTriggerSeq3(14);      // initialize ADC0, software trigger, PD1/AIN14
//  ADC0_InitSWTriggerSeq3(15);      // initialize ADC0, software trigger, PD0/AIN15
//  ADC0_InitSWTriggerSeq3(16);      // initialize ADC0, software trigger, PK0/AIN16
//  ADC0_InitSWTriggerSeq3(17);      // initialize ADC0, software trigger, PK1/AIN17
//  ADC0_InitSWTriggerSeq3(18);      // initialize ADC0, software trigger, PK2/AIN18
//  ADC0_InitSWTriggerSeq3(19);      // initialize ADC0, software trigger, PK3/AIN19
//  ADC0_InitAllTriggerSeq3(0);      // initialize ADC0, continuous trigger, PE3/AIN0
  Timer0A_Init10HzInt();           // set up Timer0A for 10 Hz interrupts
  GPIO_PORTN_DIR_R |= 0x02;        // make PN1 out (PN1 built-in LED1)
  GPIO_PORTN_AFSEL_R &= ~0x02;     // disable alt funct on PN1
  GPIO_PORTN_DEN_R |= 0x02;        // enable digital I/O on PN1
                                   // configure PN1 as GPIO
  GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF0F)+0x00000000;
  GPIO_PORTN_AMSEL_R &= ~0x02;     // disable analog functionality on PN1
  EnableInterrupts();
  while(1){
    WaitForInterrupt();
  }
}
