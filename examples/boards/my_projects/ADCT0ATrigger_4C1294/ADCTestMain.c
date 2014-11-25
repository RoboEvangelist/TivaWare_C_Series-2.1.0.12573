// ADCTestMain.c
// Runs on TM4C1294
// This program periodically samples ADC channel 0 and sets the
// brightness of LED4 according to the ADC conversion result.
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

// center of X-ohm potentiometer connected to PE3/AIN0
// bottom of X-ohm potentiometer connected to ground
// top of X-ohm potentiometer connected to +3.3V
#include <stdint.h>
#include "ADCT0ATrigger.h"
#include "tm4c1294ncpdt.h"
#include "PLL.h"

#define GPIO_PORTN1             (*((volatile uint32_t *)0x40064008))

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical(void);     // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// period is 16-bit number of PWM clock cycles in one period (3<=period)
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
// PWM clock rate = processor clock rate/SYSCTL_RCC_PWMDIV
//                = BusClock/2
//                = 120 MHz/2 = 60 MHz (in this example)
// Output on PF0/M0PWM0
void M0PWM0_Init(unsigned short period, unsigned short duty){
  SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;// 1) activate clock for PWM0
                                        // 2) activate clock for Port F
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
                                        //    allow time for clock to stabilize
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};
  GPIO_PORTF_AFSEL_R |= 0x01;           // 3) enable alt funct on PF0
  GPIO_PORTF_DEN_R |= 0x01;             //    enable digital I/O on PF0
                                        // 4) configure PF0 as M0PWM0
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFFFF0)+0x00000006;
  GPIO_PORTF_AMSEL_R &= ~0x01;          //    disable analog functionality on PF0
                                        //    allow time for clock to stabilize
  while((SYSCTL_PRPWM_R&SYSCTL_PRPWM_R0) == 0){};
                                        // 5) configure to use PWM clock divider of 2
  PWM0_CC_R = ((PWM0_CC_R&~PWM_CC_PWMDIV_M)+PWM_CC_PWMDIV_2) |
              PWM_CC_USEPWM;
  PWM0_0_CTL_R = 0;                     // 6) re-loading down-counting mode
  // PF0 goes low on LOAD
  // PF0 goes high on CMPA down
  PWM0_0_GENA_R = (PWM_0_GENA_ACTCMPAD_ONE|PWM_0_GENA_ACTLOAD_ZERO);
  PWM0_0_LOAD_R = period - 1;           // 7) cycles needed to count down to 0
  PWM0_0_CMPA_R = duty - 1;             // 8) count value when output rises
  PWM0_0_CTL_R |= PWM_0_CTL_ENABLE;     // 9) start PWM0
  PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN;   // 10) enable PF0/M0PWM0
}
// change duty cycle of PF0
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
void M0PWM0_Duty(unsigned short duty){
  PWM0_0_CMPA_R = duty - 1;             // 8) count value when output rises
}
// example user task inputs the ADC conversion result and does
// something; in this case it toggles LED1, converts the ADC
// result into a duty cycle, and sets the M0PWM0/PF0/LED4 duty
// cycle to the calculated result
void UserTask(uint32_t ADCvalue){
    GPIO_PORTN1 ^= 0x02;      // toggle LED1
    M0PWM0_Duty((ADCvalue*60000)>>12);
}
int main(void){
  PLL_Init();                 // 120 MHz
                              // activate clock for Port N
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;
                              // allow time for clock to stabilize
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};
                              // initialize ADC0, Timer0A trigger, PE3/AIN0, 10 Hz sampling
  ADC0_InitTimer0ATriggerSeq3(0, 199, 59999, &UserTask);
  M0PWM0_Init(60000, 9000);   // initialize M0PWM0 on PF0, 1,000 Hz frequency, 15% duty cycle
  GPIO_PORTN_DIR_R |= 0x02;   // make PN1 out (PN1 built-in LED1)
  GPIO_PORTN_AFSEL_R &= ~0x02;// disable alt funct on PN1
  GPIO_PORTN_DEN_R |= 0x02;   // enable digital I/O on PN1
                              // configure PN1 as GPIO
  GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF0F)+0x00000000;
  GPIO_PORTN_AMSEL_R &= ~0x02;// disable analog functionality on PN1
  GPIO_PORTN1 = 0;            // turn off LED
  EnableInterrupts();
  while(1){
    WaitForInterrupt();
  }
}
