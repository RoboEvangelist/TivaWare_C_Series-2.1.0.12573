// ADCT0ATrigger.c
// Runs on TM4C1294
// Provide a function that initializes Timer0A to trigger ADC
// SS3 conversions and request an interrupt when the conversion
// is complete.
// Daniel Valvano
// April 14, 2014

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
#include <stdint.h>

#include "ADCT0ATrigger.h"
#include "tm4c1294ncpdt.h"

void (*ADCTask)(uint32_t);   // user function

// There are many choices to make when using the ADC, and many
// different combinations of settings will all do basically the
// same thing.  For simplicity, this function makes some choices
// for you.  When calling this function, be sure that it does
// not conflict with any other software that may be running on
// the microcontroller.  Particularly, ADC0 sample sequencer 3
// is used here because it only takes one sample, and only one
// sample is absolutely needed.  Sample sequencer 3 generates a
// raw interrupt when the conversion is complete, and it is then
// promoted to an ADC0 controller interrupt.  Hardware Timer0A
// triggers the ADC0 conversion at the programmed interval, and
// software handles the interrupt to process the measurement
// when it is complete.  The user must provide a pointer to a
// function that is called in the ADC0 ISR and passes in the ADC
// conversion result.
//
// A simpler approach would be to use software to trigger the
// ADC0 conversion, wait for it to complete, and then process the
// measurement.
//
// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Timer clock source: system clock
// Timer0A: enabled
// Mode: 16-bit, down counting
// One-shot or periodic: periodic
// Prescale value: programmable using variable 'prescale' [0:255]
// Interval value: programmable using variable 'period' [0:65535]
// Sample time is busPeriod*(prescale+1)*(period+1)
// ADC clock source: 480 MHz PLL VCO / 15 = 32 MHz
//  (max sample rate: <=2,000,000 samples/second)
//  (assumes PLL_Init() has been called and not changed)
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: Timer0A
// Hardware oversampling: none
// Voltage reference: internal VDDA and GNDA
// ADC conversion sent to: FIFO (not digital comparator)
// SS3 sample and hold time: 4 ADC clock periods
// SS3 1st sample source: programmable using variable 'channelNum' [0:19]
// SS3 interrupts: enabled and used to call 'task' when sample is complete, passing in the ADC conversion result
void ADC0_InitTimer0ATriggerSeq3(unsigned char channelNum, unsigned char prescale, unsigned short period, void(*task)(uint32_t)){
  volatile uint32_t delay;
  // **** GPIO pin initialization ****
  switch(channelNum){                 // 1) activate clock
    case 0:
    case 1:
    case 2:
    case 3:
    case 8:
    case 9:                           //    these are on GPIO_PORTE
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
                                      //    allow time for clock to stabilize
      while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){};
      break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 12:
    case 13:
    case 14:
    case 15:                          //    these are on GPIO_PORTD
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
                                      //    allow time for clock to stabilize
      while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R3) == 0){};
      break;
    case 10:
    case 11:                          //    these are on GPIO_PORTB
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;
                                      //    allow time for clock to stabilize
      while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R1) == 0){};
      break;
    case 16:
    case 17:
    case 18:
    case 19:                          //    these are on GPIO_PORTK
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R9;
                                      //    allow time for clock to stabilize
      while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R9) == 0){};
      break;
    default: return;                  //    0 to 19 are valid channels on the TM4C1294
  }
  switch(channelNum){
    case 0:                           //      Ain0 is on PE3
      GPIO_PORTE_AFSEL_R |= 0x08;     // 2.0) enable alternate function on PE3
      GPIO_PORTE_DEN_R &= ~0x08;      // 3.0) disable digital I/O on PE3
      GPIO_PORTE_AMSEL_R |= 0x08;     // 4.0) enable analog functionality on PE3
      break;
    case 1:                           //      Ain1 is on PE2
      GPIO_PORTE_AFSEL_R |= 0x04;     // 2.1) enable alternate function on PE2
      GPIO_PORTE_DEN_R &= ~0x04;      // 3.1) disable digital I/O on PE2
      GPIO_PORTE_AMSEL_R |= 0x04;     // 4.1) enable analog functionality on PE2
      break;
    case 2:                           //      Ain2 is on PE1
      GPIO_PORTE_AFSEL_R |= 0x02;     // 2.2) enable alternate function on PE1
      GPIO_PORTE_DEN_R &= ~0x02;      // 3.2) disable digital I/O on PE1
      GPIO_PORTE_AMSEL_R |= 0x02;     // 4.2) enable analog functionality on PE1
      break;
    case 3:                           //      Ain3 is on PE0
      GPIO_PORTE_AFSEL_R |= 0x01;     // 2.3) enable alternate function on PE0
      GPIO_PORTE_DEN_R &= ~0x01;      // 3.3) disable digital I/O on PE0
      GPIO_PORTE_AMSEL_R |= 0x01;     // 4.3) enable analog functionality on PE0
      break;
    case 4:                           //      Ain4 is on PD7
      GPIO_PORTD_LOCK_R = 0x4C4F434B; //      unlock GPIO_PORTD
      GPIO_PORTD_CR_R |= 0x80;        //      allow access to PD7
      GPIO_PORTD_AFSEL_R |= 0x80;     // 2.4) enable alternate function on PD7
      GPIO_PORTD_DEN_R &= ~0x08;      // 3.4) disable digital I/O on PD7
      GPIO_PORTD_AMSEL_R |= 0x08;     // 4.4) enable analog functionality on PD7
      break;
    case 5:                           //      Ain5 is on PD6
      GPIO_PORTD_AFSEL_R |= 0x40;     // 2.5) enable alternate function on PD6
      GPIO_PORTD_DEN_R &= ~0x40;      // 3.5) disable digital I/O on PD6
      GPIO_PORTD_AMSEL_R |= 0x40;     // 4.5) enable analog functionality on PD6
      break;
    case 6:                           //      Ain6 is on PD5
      GPIO_PORTD_AFSEL_R |= 0x20;     // 2.6) enable alternate function on PD5
      GPIO_PORTD_DEN_R &= ~0x20;      // 3.6) disable digital I/O on PD5
      GPIO_PORTD_AMSEL_R |= 0x20;     // 4.6) enable analog functionality on PD5
      break;
    case 7:                           //      Ain7 is on PD4
      GPIO_PORTD_AFSEL_R |= 0x10;     // 2.7) enable alternate function on PD4
      GPIO_PORTD_DEN_R &= ~0x10;      // 3.7) disable digital I/O on PD4
      GPIO_PORTD_AMSEL_R |= 0x10;     // 4.7) enable analog functionality on PD4
      break;
    case 8:                           //      Ain8 is on PE5
      GPIO_PORTE_AFSEL_R |= 0x20;     // 2.8) enable alternate function on PE5
      GPIO_PORTE_DEN_R &= ~0x20;      // 3.8) disable digital I/O on PE5
      GPIO_PORTE_AMSEL_R |= 0x20;     // 4.8) enable analog functionality on PE5
      break;
    case 9:                           //      Ain9 is on PE4
      GPIO_PORTE_AFSEL_R |= 0x10;     // 2.9) enable alternate function on PE4
      GPIO_PORTE_DEN_R &= ~0x10;      // 3.9) disable digital I/O on PE4
      GPIO_PORTE_AMSEL_R |= 0x10;     // 4.9) enable analog functionality on PE4
      break;
    case 10:                          //       Ain10 is on PB4
      GPIO_PORTB_AFSEL_R |= 0x10;     // 2.10) enable alternate function on PB4
      GPIO_PORTB_DEN_R &= ~0x10;      // 3.10) disable digital I/O on PB4
      GPIO_PORTB_AMSEL_R |= 0x10;     // 4.10) enable analog functionality on PB4
      break;
    case 11:                          //       Ain11 is on PB5
      GPIO_PORTB_AFSEL_R |= 0x20;     // 2.11) enable alternate function on PB5
      GPIO_PORTB_DEN_R &= ~0x20;      // 3.11) disable digital I/O on PB5
      GPIO_PORTB_AMSEL_R |= 0x20;     // 4.11) enable analog functionality on PB5
      break;
    case 12:                          //       Ain12 is on PD3
      GPIO_PORTD_AFSEL_R |= 0x08;     // 2.12) enable alternate function on PD3
      GPIO_PORTD_DEN_R &= ~0x08;      // 3.12) disable digital I/O on PD3
      GPIO_PORTD_AMSEL_R |= 0x08;     // 4.12) enable analog functionality on PD3
      break;
    case 13:                          //       Ain13 is on PD2
      GPIO_PORTD_AFSEL_R |= 0x04;     // 2.13) enable alternate function on PD2
      GPIO_PORTD_DEN_R &= ~0x04;      // 3.13) disable digital I/O on PD2
      GPIO_PORTD_AMSEL_R |= 0x04;     // 4.13) enable analog functionality on PD2
      break;
    case 14:                          //       Ain14 is on PD1
      GPIO_PORTD_AFSEL_R |= 0x02;     // 2.14) enable alternate function on PD1
      GPIO_PORTD_DEN_R &= ~0x02;      // 3.14) disable digital I/O on PD1
      GPIO_PORTD_AMSEL_R |= 0x02;     // 4.14) enable analog functionality on PD1
      break;
    case 15:                          //       Ain15 is on PD0
      GPIO_PORTD_AFSEL_R |= 0x01;     // 2.15) enable alternate function on PD0
      GPIO_PORTD_DEN_R &= ~0x01;      // 3.15) disable digital I/O on PD0
      GPIO_PORTD_AMSEL_R |= 0x01;     // 4.15) enable analog functionality on PD0
      break;
    case 16:                          //       Ain16 is on PK0
      GPIO_PORTK_AFSEL_R |= 0x01;     // 2.16) enable alternate function on PK0
      GPIO_PORTK_DEN_R &= ~0x01;      // 3.16) disable digital I/O on PK0
      GPIO_PORTK_AMSEL_R |= 0x01;     // 4.16) enable analog functionality on PK0
      break;
    case 17:                          //       Ain17 is on PK1
      GPIO_PORTK_AFSEL_R |= 0x02;     // 2.17) enable alternate function on PK1
      GPIO_PORTK_DEN_R &= ~0x02;      // 3.17) disable digital I/O on PK1
      GPIO_PORTK_AMSEL_R |= 0x02;     // 4.17) enable analog functionality on PK1
      break;
    case 18:                          //       Ain18 is on PK2
      GPIO_PORTK_AFSEL_R |= 0x04;     // 2.18) enable alternate function on PK2
      GPIO_PORTK_DEN_R &= ~0x04;      // 3.18) disable digital I/O on PK2
      GPIO_PORTK_AMSEL_R |= 0x04;     // 4.18) enable analog functionality on PK2
      break;
    case 19:                          //       Ain19 is on PK3
      GPIO_PORTK_AFSEL_R |= 0x08;     // 2.19) enable alternate function on PK3
      GPIO_PORTK_DEN_R &= ~0x08;      // 3.19) disable digital I/O on PK3
      GPIO_PORTK_AMSEL_R |= 0x08;     // 4.19) enable analog functionality on PK3
      break;
  }
  // **** general initialization ****
                                      // 5) activate timer0
  SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;
                                      //    allow time for clock to stabilize
  while((SYSCTL_PRTIMER_R&SYSCTL_PRTIMER_R0) == 0){};
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN;    // 6) disable timer0A during setup
  TIMER0_CTL_R |= TIMER_CTL_TAOTE;    // 7) enable timer0A trigger to ADC
  TIMER0_ADCEV_R |= TIMER_ADCEV_TATOADCEN;//timer0A time-out event ADC trigger enabled
  TIMER0_CFG_R = TIMER_CFG_16_BIT;    // 8) configure for 16-bit timer mode
  TIMER0_CC_R &= ~TIMER_CC_ALTCLK;    // 9) timer0 clocked from system clock
  // **** timer0A initialization ****
                                      // 10) configure for periodic mode, default down-count settings
  TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
  TIMER0_TAPR_R = prescale;           // 11) prescale value for trigger
  TIMER0_TAILR_R = period;            // 12) start value for trigger
  TIMER0_IMR_R &= ~TIMER_IMR_TATOIM;  // 13) disable timeout (rollover) interrupt
  TIMER0_CTL_R |= TIMER_CTL_TAEN;     // 14) enable timer0A 16-b, periodic, no interrupts
  // **** ADC initialization ****
                                      // 15) activate clock for ADC0
  SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
                                      //     allow time for clock to stabilize
  while((SYSCTL_PRADC_R&SYSCTL_PRADC_R0) == 0){};
                                      // 16) configure ADC clock source as PLL VCO / 15 = 32 MHz
  ADC0_CC_R = ((ADC0_CC_R&~ADC_CC_CLKDIV_M)+(14<<ADC_CC_CLKDIV_S)) |
              ((ADC0_CC_R&~ADC_CC_CS_M)+ADC_CC_CS_SYSPLL);
  ADC0_SSPRI_R = 0x3210;              // 17) sequencer 3 is lowest priority (default)
  ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;   // 18) disable sample sequencer 3
                                      // 19) configure seq3 for timer trigger
  ADC0_EMUX_R = (ADC0_EMUX_R&~ADC_EMUX_EM3_M)+ADC_EMUX_EM3_TIMER;
                                      // 20) configure for no hardware oversampling (default)
  ADC0_SAC_R = (ADC0_SAC_R&~ADC_SAC_AVG_M)+ADC_SAC_AVG_OFF;
                                      // 21) configure for internal reference (default)
  ADC0_CTL_R = (ADC0_CTL_R&~ADC_CTL_VREF_M)+ADC_CTL_VREF_INTERNAL;
  ADC0_SSOP3_R &= ~ADC_SSOP3_S0DCOP;  // 22) configure for ADC result saved to FIFO (default)
                                      // 23) configure for 4 ADC clock period S&H (default)
  ADC0_SSTSH3_R = (ADC0_SSTSH3_R&~ADC_SSTSH3_TSH0_M)+0;
                                      // 24) set channel
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&~0x000F)+(channelNum&0xF);
                                      // 25) set extended channel
  ADC0_SSEMUX3_R = (ADC0_SSEMUX3_R&~0x01)+((channelNum&0x10)>>4);
  ADC0_SSCTL3_R = 0x0006;             // 26) no TS0 D0, yes IE0 END0
  ADC0_IM_R |= ADC_IM_MASK3;          // 27) enable SS3 interrupts
  ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;    // 28) enable sample sequencer 3
  // **** interrupt initialization ****
                                      // 29) ADC3=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF00FF)|0x00004000; // bits 13-15
  NVIC_EN0_R = 0x00020000;            // 30) enable interrupt 17 in NVIC
  ADCTask = task;                     // 31) link user task
}

void ADC0Seq3_Handler(void){
  ADC0_ISC_R = ADC_ISC_IN3;           // acknowledge ADC sequence 3 completion
                                      // execute user task
  (*ADCTask)(ADC0_SSFIFO3_R&ADC_SSFIFO3_DATA_M);
}
