// ADCSWTrigger.h
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
void ADC0_InitSWTriggerSeq3_Ch0(void);

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
void ADC0_InitSWTriggerSeq3(unsigned char channelNum);

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
void ADC0_InitAllTriggerSeq3(unsigned char channelNum);

//------------ADC0_InSeq3------------
// Busy-wait analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
uint32_t ADC0_InSeq3(void);
