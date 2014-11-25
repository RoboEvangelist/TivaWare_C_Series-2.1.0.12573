// ADCT0ATrigger.h
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
void ADC0_InitTimer0ATriggerSeq3(unsigned char channelNum, unsigned char prescale, unsigned short period, void(*task)(uint32_t));
