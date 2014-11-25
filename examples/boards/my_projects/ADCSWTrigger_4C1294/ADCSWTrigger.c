// ADCSWTrigger.c
// Runs on TM4C1294
// Provide functions that initialize ADC0 SS3 to be triggered by
// software and trigger a conversion, wait for it to finish,
// and return the result.
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
#include <stdint.h>
#include "ADCSWTrigger.h"
#include "tm4c1294ncpdt.h"

// There are many choices to make when using the ADC, and many
// different combinations of settings will all do basically the
// same thing.  For simplicity, this function makes some choices
// for you.  When calling this function, be sure that it does
// not conflict with any other software that may be running on
// the microcontroller.  Particularly, ADC0 sample sequencer 3
// is used here because it only takes one sample, and only one
// sample is absolutely needed.  Sample sequencer 3 generates a
// raw interrupt when the conversion is complete, but it is not
// promoted to a controller interrupt.  Software triggers the
// ADC0 conversion and waits for the conversion to finish.  If
// somewhat precise periodic measurements are required, the
// software trigger can occur in a periodic interrupt.  This
// approach has the advantage of being simple.  However, it does
// not guarantee real-time.
//
// A better approach would be to use a hardware timer to trigger
// the ADC0 conversion independently from software and generate
// an interrupt when the conversion is finished.  Then, the
// software can transfer the conversion result to memory and
// process it after all measurements are complete.

// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// ADC clock source: 480 MHz PLL VCO / 15 = 32 MHz
//  (max sample rate: <=2,000,000 samples/second)
//  (assumes PLL_Init() has been called and not changed)
// Sequencer 0 priority: 4th (lowest)
// Sequencer 1 priority: 3rd
// Sequencer 2 priority: 2nd
// Sequencer 3 priority: 1st (highest)
// SS3 triggering event: software trigger
// Hardware oversampling: none
// Voltage reference: internal VDDA and GNDA
// ADC conversion sent to: FIFO (not digital comparator)
// SS3 sample and hold time: 4 ADC clock periods
// SS3 1st sample source: Ain0 (PE3)
// SS3 interrupts: enabled but not promoted to controller
void ADC0_InitSWTriggerSeq3_Ch0(void){
                                  // 1) activate clock for Port E
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
                                  // allow time for clock to stabilize
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){};
  GPIO_PORTE_AFSEL_R |= 0x08;     // 2) enable alternate function on PE3
  GPIO_PORTE_DEN_R &= ~0x08;      // 3) disable digital I/O on PE3
  GPIO_PORTE_AMSEL_R |= 0x08;     // 4) enable analog functionality on PE3
                                  // 5) activate clock for ADC0
  SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
                                  // allow time for clock to stabilize
  while((SYSCTL_PRADC_R&SYSCTL_PRADC_R0) == 0){};
                                  // 6) configure ADC clock source as PLL VCO / 15 = 32 MHz
  ADC0_CC_R = ((ADC0_CC_R&~ADC_CC_CLKDIV_M)+(14<<ADC_CC_CLKDIV_S)) |
              ((ADC0_CC_R&~ADC_CC_CS_M)+ADC_CC_CS_SYSPLL);
  ADC0_SSPRI_R = 0x0123;          // 7) sequencer 3 is highest priority
                                  // 8) disable sample sequencer 3
  ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;
                                  // 9) configure seq3 for software trigger (default)
  ADC0_EMUX_R = (ADC0_EMUX_R&~ADC_EMUX_EM3_M)+ADC_EMUX_EM3_PROCESSOR;
                                  // 10) configure for no hardware oversampling (default)
  ADC0_SAC_R = (ADC0_SAC_R&~ADC_SAC_AVG_M)+ADC_SAC_AVG_OFF;
                                  // 11) configure for internal reference (default)
  ADC0_CTL_R = (ADC0_CTL_R&~ADC_CTL_VREF_M)+ADC_CTL_VREF_INTERNAL;
                                  // 12) configure for ADC result saved to FIFO (default)
  ADC0_SSOP3_R &= ~ADC_SSOP3_S0DCOP;
                                  // 13) configure for 4 ADC clock period S&H (default)
  ADC0_SSTSH3_R = (ADC0_SSTSH3_R&~ADC_SSTSH3_TSH0_M)+0;
                                  // 14) set channel
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&~0x000F)+0;
  ADC0_SSEMUX3_R &= ~0x01;        // 15) SS3 in range 0:15
  ADC0_SSCTL3_R = 0x0006;         // 16) no TS0 D0, yes IE0 END0
  ADC0_IM_R &= ~ADC_IM_MASK3;     // 17) disable SS3 interrupts
  ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;// 18) enable sample sequencer 3
}

// This helper function initializes the proper GPIO pin for the
// given ADC channel number.
static void gpio2adc(unsigned char channelNum){
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
}

// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// ADC clock source: 480 MHz PLL VCO / 15 = 32 MHz
//  (max sample rate: <=2,000,000 samples/second)
//  (assumes PLL_Init() has been called and not changed)
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: software trigger
// Hardware oversampling: none
// Voltage reference: internal VDDA and GNDA
// ADC conversion sent to: FIFO (not digital comparator)
// SS3 sample and hold time: 4 ADC clock periods
// SS3 1st sample source: programmable using variable 'channelNum' [0:19]
// SS3 interrupts: enabled but not promoted to controller
void ADC0_InitSWTriggerSeq3(unsigned char channelNum){
  if(channelNum > 19){
    return;                       // 0 to 19 are valid channels on the TM4C1294
  }
  gpio2adc(channelNum);           // 1-4) initialize GPIO pins
                                  // 5) activate clock for ADC0
  SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
                                  // allow time for clock to stabilize
  while((SYSCTL_PRADC_R&SYSCTL_PRADC_R0) == 0){};
                                  // 6) configure ADC clock source as PLL VCO / 15 = 32 MHz
  ADC0_CC_R = ((ADC0_CC_R&~ADC_CC_CLKDIV_M)+(14<<ADC_CC_CLKDIV_S)) |
              ((ADC0_CC_R&~ADC_CC_CS_M)+ADC_CC_CS_SYSPLL);
  ADC0_SSPRI_R = 0x3210;          // 7) sequencer 3 is lowest priority (default)
                                  // 8) disable sample sequencer 3
  ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;
                                  // 9) configure seq3 for software trigger (default)
  ADC0_EMUX_R = (ADC0_EMUX_R&~ADC_EMUX_EM3_M)+ADC_EMUX_EM3_PROCESSOR;
                                  // 10) configure for no hardware oversampling (default)
  ADC0_SAC_R = (ADC0_SAC_R&~ADC_SAC_AVG_M)+ADC_SAC_AVG_OFF;
                                  // 11) configure for internal reference (default)
  ADC0_CTL_R = (ADC0_CTL_R&~ADC_CTL_VREF_M)+ADC_CTL_VREF_INTERNAL;
                                  // 12) configure for ADC result saved to FIFO (default)
  ADC0_SSOP3_R &= ~ADC_SSOP3_S0DCOP;
                                  // 13) configure for 4 ADC clock period S&H (default)
  ADC0_SSTSH3_R = (ADC0_SSTSH3_R&~ADC_SSTSH3_TSH0_M)+0;
                                  // 14) set channel
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&~0x000F)+(channelNum&0xF);
                                  // 15) set extended channel
  ADC0_SSEMUX3_R = (ADC0_SSEMUX3_R&~0x01)+((channelNum&0x10)>>4);
  ADC0_SSCTL3_R = 0x0006;         // 16) no TS0 D0, yes IE0 END0
  ADC0_IM_R &= ~ADC_IM_MASK3;     // 17) disable SS3 interrupts
  ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;// 18) enable sample sequencer 3
}

// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// ADC clock source: 480 MHz PLL VCO / 15 = 32 MHz
//  (max sample rate: <=2,000,000 samples/second)
//  (assumes PLL_Init() has been called and not changed)
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: always trigger
// Hardware oversampling: none
// Voltage reference: internal VDDA and GNDA
// ADC conversion sent to: FIFO (not digital comparator)
// SS3 sample and hold time: 4 ADC clock periods
// SS3 1st sample source: programmable using variable 'channelNum' [0:19]
// SS3 interrupts: enabled but not promoted to controller
void ADC0_InitAllTriggerSeq3(unsigned char channelNum){
  if(channelNum > 19){
    return;                       // 0 to 19 are valid channels on the TM4C1294
  }
  gpio2adc(channelNum);           // 1-4) initialize GPIO pins
                                  // 5) activate clock for ADC0
  SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
                                  // allow time for clock to stabilize
  while((SYSCTL_PRADC_R&SYSCTL_PRADC_R0) == 0){};
                                  // 6) configure ADC clock source as PLL VCO / 15 = 32 MHz
  ADC0_CC_R = ((ADC0_CC_R&~ADC_CC_CLKDIV_M)+(14<<ADC_CC_CLKDIV_S)) |
              ((ADC0_CC_R&~ADC_CC_CS_M)+ADC_CC_CS_SYSPLL);
  ADC0_SSPRI_R = 0x3210;          // 7) sequencer 3 is lowest priority (default)
                                  // 8) disable sample sequencer 3
  ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;
                                  // 9) configure seq3 for continuous triggering
  ADC0_EMUX_R = (ADC0_EMUX_R&~ADC_EMUX_EM3_M)+ADC_EMUX_EM3_ALWAYS;
                                  // 10) configure for no hardware oversampling (default)
  ADC0_SAC_R = (ADC0_SAC_R&~ADC_SAC_AVG_M)+ADC_SAC_AVG_OFF;
                                  // 11) configure for internal reference (default)
  ADC0_CTL_R = (ADC0_CTL_R&~ADC_CTL_VREF_M)+ADC_CTL_VREF_INTERNAL;
                                  // 12) configure for ADC result saved to FIFO (default)
  ADC0_SSOP3_R &= ~ADC_SSOP3_S0DCOP;
                                  // 13) configure for 4 ADC clock period S&H (default)
  ADC0_SSTSH3_R = (ADC0_SSTSH3_R&~ADC_SSTSH3_TSH0_M)+0;
                                  // 14) set channel
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&~0x000F)+(channelNum&0xF);
                                  // 15) set extended channel
  ADC0_SSEMUX3_R = (ADC0_SSEMUX3_R&~0x01)+((channelNum&0x10)>>4);
  ADC0_SSCTL3_R = 0x0006;         // 16) no TS0 D0, yes IE0 END0
  ADC0_IM_R &= ~ADC_IM_MASK3;     // 17) disable SS3 interrupts
  ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;// 18) enable sample sequencer 3
}

//------------ADC0_InSeq3------------
// Busy-wait analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
uint32_t ADC0_InSeq3(void){  uint32_t result;
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
  ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
  return result;
}
