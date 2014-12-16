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
#include "driverlib/udma.h"
#include "inc/hw_adc.h"

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t ui8ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(ui8ControlTable, 1024)
uint8_t ui8ControlTable[1024];
#else
uint8_t ui8ControlTable[1024] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The pair of ping-pong buffers for the converted results from ADC0 SS0.
//
//*****************************************************************************
// we'll get 2 samples over 100 milli-seconds, since we are only using 4 SS samples per
// 100 ms
#define ADC_BUF_SIZE	2
static uint32_t ui32BufA[ADC_BUF_SIZE];
static uint32_t ui32BufB[ADC_BUF_SIZE];

//*****************************************************************************
//
// The count of ADC buffers filled, one for each ping-pong buffer.
//
//*****************************************************************************
static uint32_t ui32BufACount = 0;
static uint32_t ui32BufBCount = 0;

// variables for the left and right IR sensors
static uint32_t ui32IRValues[ADC_BUF_SIZE];              // each value represents a sensor data
volatile uint32_t ui32LeftSensor;      // PE3
volatile uint32_t ui32RightSensor;     // PE4
//volatile uint32_t ui32FrontSensor;     // PE5
volatile uint32_t ui32SensorsDiff;     // difference between Left and Right sensor

// minimum obstacle avoidance threshold in cm
volatile const uint8_t ui8ObstacleDistance = 30;
// time delay
volatile const uint32_t ui32TimeDelay = 1000000;

// use these booleans to avoid sensing the same command over and over
bool boolMovingForward = false;
bool boolTurningLeft = false;
bool boolTurningRight = false;
bool boolMovingBack = false;
bool boolStopped = false;

void ADC0_Init(void){ 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);   // activate the clock of ADC0
	while((SYSCTL_PRADC_R&SYSCTL_PRADC_R0) == 0){};
		
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  // activate the clock of E
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){};
		
	// 5) activate timer0
	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;
																						//    allow time for clock to stabilize
	while((SYSCTL_PRTIMER_R&SYSCTL_PRTIMER_R0) == 0){};
	TIMER0_CTL_R &= ~TIMER_CTL_TAEN;          // 6) disable timer0A during setup
	TIMER0_CTL_R |= TIMER_CTL_TAOTE;          // 7) enable timer0A trigger to ADC
	TIMER0_ADCEV_R |= TIMER_ADCEV_TATOADCEN;  //timer0A time-out event ADC trigger enabled
	TIMER0_CFG_R = 0x00000000;					      // 8) configure for 32-bit timer mode
	TIMER0_CC_R &= ~TIMER_CC_ALTCLK;          // 9) timer0 clocked from system clock
		
	// **** timer0A initialization ****
																		// 10) configure for periodic mode, default down-count settings
	TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
	TIMER0_TAPR_R = 199;           // 11) prescale value for trigger
	TIMER0_TAILR_R = 4000000;            // 12) start value for trigger
	TIMER0_IMR_R &= ~TIMER_IMR_TATOIM;  // 13) disable timeout (rollover) interrupt
	TIMER0_CTL_R |= TIMER_CTL_TAEN;     // 14) enable timer0A 32-b, periodic, no interrupts

	
	// Enable PE3 PE4 PE5 as analog	
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 );
		
	ADCSequenceDisable(ADC0_BASE, 1); //disable ADC0 before the configuration is complete
		
	// use ADC0, SS1 (4 samples max), timer trigger, priority 3
	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_TIMER, 1);
		
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH0); // PE3/analog Input 0 - Left sensor
	//ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH9|ADC_CTL_IE|ADC_CTL_END); // PE4/analog Input 9  - Right sensor
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_CH8|ADC_CTL_IE|ADC_CTL_END);  // PE5/analog Input 8 - Middle sensor
	//ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_CH8|ADC_CTL_IE|ADC_CTL_END);  // P	`E5/analog Input 8 - Middle sensor
		
	IntPrioritySet(INT_ADC0SS1, 0x01);  	 // configure ADC0 SS1 interrupt priority as 1
	IntEnable(INT_ADC0SS1);    				// enable interrupt 31 in NVIC (ADC0 SS1)
	ADCIntEnableEx(ADC0_BASE, ADC_INT_SS1);      // arm interrupt of ADC0 SS1
	
	ADCSequenceDMAEnable(ADC0_BASE, 1);     		 //enable DMA for ADC0 SS1
	ADCSequenceEnable(ADC0_BASE, 1);
}

//DMA initializaiton
void DMA_Init(void)
{
		
		//----- 1. Module Initialization Start -----//
		//
    // Enable the uDMA clock
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
		
		//
    // Enable the uDMA controller by setting the MASTEREN bit of 
		// the DMA Configuration (DMA_CFG) register.
    //
    uDMAEnable();

    //
    // Point at the control table to use for channel control structures.
    // Program the location of the channel control table by writing the base address of the table to the
		// DMA Channel Control Base Pointer (DMACTLBASE) register. The base address must be
		// aligned on a 1024-byte boundary.
    uDMAControlBaseSet(ui8ControlTable);
	
		//----- 1. End -----//
		
		
		//----- 2. Configure the Channel Attributes -----//
    // Put the attributes in a known state for the uDMA ADC0 SS0 channel.  These
    // should already be disabled by default.
    // 1. Clear ALTSETLECT: for Ping-Pong mode, the uDMA controller automatically configures
		//											the ALT bit, so just use the default value
		// 2. Clear HIGH_PRIORITY: use default priority
		// 3. Clear REQMASK: allow the uDMA controller to recognize requests for this channel
    //uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC1,
		uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC0,
                                UDMA_ATTR_ALTSELECT |
                                UDMA_ATTR_HIGH_PRIORITY |
                                UDMA_ATTR_REQMASK);		
																
		// 4. Set USEBURST: configure the uDMA controller to respond to burst requests only
		//uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC1,
		uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC0,
                                UDMA_ATTR_USEBURST);	
		//----- 2. End -----//														


    //----- 3. Configure the control parameters of the channel control structure-----//
    // Configure the control parameters for the primary control structure for
    // the ADC0 SS0 channel.  The primary contol structure is used for the "A"
    // part of the ping-pong receive.  The transfer data size is 32 bits, the
    // source address does not increment since it will be reading from a
    // register.  The destination address increment is 32 bits (4 bytes).  The
    // arbitration size is set to 2 to match the ADC0 SS0 half FIFO size.
    //
    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                          UDMA_SIZE_32 | UDMA_SRC_INC_NONE | UDMA_DST_INC_32 |
                          UDMA_ARB_2);

    //
    // Configure the control parameters for the alternate control structure for
    // the ADC0 SS0 channel.  The alternate contol structure is used for the "B"
    // part of the ping-pong receive.  The configuration is identical to the
    // primary/A control structure.
    //
    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                          UDMA_SIZE_32 | UDMA_SRC_INC_NONE | UDMA_DST_INC_32 |
                          UDMA_ARB_2);
		//----- 3. End -----//
		
		//----- 4. Configure the transfer parameters of the channel control structure-----//
    // Set up the transfer parameters for the ADC0 SS0 primary control
    // structure.  The mode is set to ping-pong, the transfer source is the
    // ADC0 SS0 FIFO result register, and the destination is the receive "A" buffer.  The
    // transfer size is set to match the size of the buffer.
    //
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)(ADC0_BASE + ADC_O_SSFIFO1),
                               ui32BufA, ADC_BUF_SIZE);

    //
    // Set up the transfer parameters for the ADC0 SS0 alternate control
    // structure.  The mode is set to ping-pong, the transfer source is the
    // ADC0 SS1 FIFO result register, and the destination is the receive "B" buffer.  The
    // transfer size is set to match the size of the buffer.
    //
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)(ADC0_BASE + ADC_O_SSFIFO1),
                               ui32BufB, ADC_BUF_SIZE);
		//----- 4. End -----//
															 
															 
    //----- 5. Enable the uDMA channel-----//
    // Now the channel is configured and is ready to start.
		// As soon as the channel is enabled, the peripheral will
    // issue a transfer request and the data transfers will begin.
    //
    uDMAChannelEnable(UDMA_CHANNEL_ADC0);
		//----- 5. End -----//													 
}

/*
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
*/


void ADC0_Handler(void)
{
		uint32_t ui32Status;	
		uint32_t ui32Mode;

    ui32Status = ADCIntStatus(ADC0_BASE, 1, false);
		ADCIntClear(ADC0_BASE, 1);
	
	    //
    // Check the DMA control table to see if the ping-pong "A" transfer is
    // complete.  The "A" transfer uses receive buffer "A", and the primary
    // control structure.
    //
    ui32Mode = uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT);

    //
    // If the primary control structure indicates stop, that means the "A"
    // receive buffer is done.  The uDMA controller should still be receiving
    // data into the "B" buffer.
    //
    if(ui32Mode == UDMA_MODE_STOP)
    {
        //
        // Increment a counter to indicate data was received into buffer A.  In
        // a real application this would be used to signal the main thread that
        // data was received so the main thread can process the data.
        //
        ui32BufACount++;
				
				ui32LeftSensor = 1.0/((37.0/648000)*ui32BufA[0]*1.0-(67.0/6480.0));
				//ui32RightSensor = 1.0/(powf(7.0, -5)*ui32IRValues[1]*1.0 - 0.0022);
				ui32RightSensor = 1.0/((37.0/648000)*ui32BufA[1]*1.0-(67.0/6480.0));
				
				// get difference in reading between motors only when an object is close enough
				if (ui32RightSensor <= 50 || ui32LeftSensor <= 50)  // if less than 60 cm
					ui32SensorsDiff = ui32RightSensor - ui32LeftSensor;
				else 
					ui32SensorsDiff = 0;
						
        //
        // Set up the next transfer for the "A" buffer, using the primary
        // control structure.  When the ongoing receive into the "B" buffer is
        // done, the uDMA controller will switch back to this one.  This
        // example re-uses buffer A, but a more sophisticated application could
        // use a rotating set of buffers to increase the amount of time that
        // the main thread has to process the data in the buffer before it is
        // reused.
        //
        uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)(ADC0_BASE + ADC_O_SSFIFO1),
                               ui32BufA, ADC_BUF_SIZE);
				//Re-enable the uDMA channel											 
				uDMAChannelEnable(UDMA_CHANNEL_ADC0);
    }

    //
    // Check the DMA control table to see if the ping-pong "B" transfer is
    // complete.  The "B" transfer uses receive buffer "B", and the alternate
    // control structure.
    //
    ui32Mode = uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT);

    //
    // If the alternate control structure indicates stop, that means the "B"
    // receive buffer is done.  The uDMA controller should still be receiving
    // data into the "A" buffer.
    //
    if(ui32Mode == UDMA_MODE_STOP)
    {
        //
        // Increment a counter to indicate data was received into buffer A.  In
        // a real application this would be used to signal the main thread that
        // data was received so the main thread can process the data.
        //
        ui32BufBCount++;
				
				ui32LeftSensor = 1.0/((37.0/648000)*ui32BufB[0]*1.0-(67.0/6480.0));
				//ui32RightSensor = 1.0/(powf(7.0, -5)*ui32IRValues[1]*1.0 - 0.0022);
				ui32RightSensor = 1.0/((37.0/648000)*ui32BufB[1]*1.0-(67.0/6480.0));
				
				// get difference in reading between motors only when an object is close enough
				if (ui32RightSensor <= 50 || ui32LeftSensor <= 50)  // if less than 60 cm
					ui32SensorsDiff = ui32RightSensor - ui32LeftSensor;
				else 
					ui32SensorsDiff = 0;
				
        //
        // Set up the next transfer for the "B" buffer, using the alternate
        // control structure.  When the ongoing receive into the "A" buffer is
        // done, the uDMA controller will switch back to this one.  This
        // example re-uses buffer B, but a more sophisticated application could
        // use a rotating set of buffers to increase the amount of time that
        // the main thread has to process the data in the buffer before it is
        // reused.
        //
        uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)(ADC0_BASE + ADC_O_SSFIFO1),
                               ui32BufB, ADC_BUF_SIZE);
				//Re-enable the uDMA channel											 
				uDMAChannelEnable(UDMA_CHANNEL_ADC0);
		}
}