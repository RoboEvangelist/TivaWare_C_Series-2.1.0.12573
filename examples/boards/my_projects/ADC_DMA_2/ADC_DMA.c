#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "inc/hw_adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/udma.h"
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/uart.h"
#include "driverlib/debug.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//!
//! In this project we use ADC0, SS0 to measure the data from the on-chip 
//! temperature sensor. The ADC sampling is triggered by software whenever 
//! four samples have been collected. Both the Celsius and the Fahreheit 
//! temperatures are calcuated.
//
//*****************************************************************************

volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;

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
// we'll get 200 samples over 5 seconds, since we are only using 4 SS samples per
// 100 ms
#define ADC_BUF_SIZE	200
static uint32_t ui32BufA[ADC_BUF_SIZE];
static uint32_t ui32BufB[ADC_BUF_SIZE];

//*****************************************************************************
//
// The count of ADC buffers filled, one for each ping-pong buffer.
//
//*****************************************************************************
static uint32_t ui32BufACount = 0;
static uint32_t ui32BufBCount = 0;

//ADC0 initializaiton
void ADC0_Init(unsigned char prescale, unsigned int period)
{
		int ui32SysClkFreq;
		ui32SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 40000000); // configure the system clock to be 40MHz
	  
	  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);	//activate the clock of ADC0
		SysCtlDelay(2);	//insert a few cycles after enabling the peripheral to allow the clock to be fully activated.
	
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
		TIMER0_TAPR_R = prescale;           // 11) prescale value for trigger
		TIMER0_TAILR_R = period;            // 12) start value for trigger
		TIMER0_IMR_R &= ~TIMER_IMR_TATOIM;  // 13) disable timeout (rollover) interrupt
		TIMER0_CTL_R |= TIMER_CTL_TAEN;     // 14) enable timer0A 32-b, periodic, no interrupts

		// **** ADC initialization ****
		ADCSequenceDisable(ADC0_BASE, 0); 	//disable ADC0 before the configuration is complete
		ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0); // will use ADC0, SS0, timer sampling, priority 0
	
		// We'll use only four steps, and all of them sample from the temperature sensor
		// This 4 steps gives us 4 samples / 100 ms = 200 samples  / 5 seconds
		ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_TS);            //ADC0 SS0 Step 0, sample from internal temperature sensor
		ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_TS|ADC_CTL_IE); //ADC0 SS0 Step 1, sample from internal temperature sensor, completion of this step will set RIS
		ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_TS);            //ADC0 SS0 Step 2, sample from internal temperature sensor
		//ADC0 SS0 Step 3, sample from internal temperature sensor, completion of this step will set RIS, last sample of the sequence
		ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END); 
	
		IntPrioritySet(INT_ADC0SS0, 0x00);  	 			 // configure ADC0 SS0 interrupt priority as 0
		IntEnable(INT_ADC0SS0);    				     			 // enable interrupt 31 in NVIC (ADC0 SS1)
		ADCIntEnableEx(ADC0_BASE, ADC_INT_SS0);      // arm interrupt of ADC0 SS0
		
		ADCSequenceDMAEnable(ADC0_BASE, 0);     		 //enable DMA for ADC0 SS0
		ADCSequenceEnable(ADC0_BASE, 0);        		 //enable ADC0
		
		// Use the LED for timer and interrupt debugging
	
		// initiate ports
		volatile uint32_t ui32Loop;
		// Enable the clock of the GPIO port N that is used for the on-board LED D1 and GPIO Port J that is used for SW1.
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R12;
    ui32Loop = SYSCTL_RCGCGPIO_R;

		// Set the direction of PN1 (LED D1) and PN0 (LED D0) as output
    GPIO_PORTN_DIR_R |= 0x03;
		// Enable PN1 and PN0 for digital function.
    GPIO_PORTN_DEN_R |= 0x03;
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
                               (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                               ui32BufA, ADC_BUF_SIZE);

    //
    // Set up the transfer parameters for the ADC0 SS0 alternate control
    // structure.  The mode is set to ping-pong, the transfer source is the
    // ADC0 SS1 FIFO result register, and the destination is the receive "B" buffer.  The
    // transfer size is set to match the size of the buffer.
    //
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)(ADC0_BASE + ADC_O_SSFIFO0),
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
		
//*****************************************************************************
//
// The interrupt handler for ADC0 SS0.  This interrupt will occur when a DMA
// transfer is complete using the ADC0 SS0 uDMA channel.  It will also be
// triggered if the peripheral signals an error.  This interrupt handler will
// switch between receive ping-pong buffers A and B. This will keep the ADC
// running continuously.
//
//*****************************************************************************
void ADC0_Handler(void)
{
		uint32_t ui32Status;	
		uint32_t ui32Mode;

    ui32Status = ADCIntStatus(ADC0_BASE, 0, false);
		ADCIntClear(ADC0_BASE, 0);
	
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

				ui32TempAvg = 0;         // clear the previous value
				// get the average temperature for buffer A
				for (int i = 0; i < ADC_BUF_SIZE; i++)
				{
					ui32TempAvg = ui32TempAvg + ui32BufA[i];
				}
				// take the average of ADC_BUF_SIZE samples over the 5 seconds
				ui32TempAvg = ui32TempAvg / ADC_BUF_SIZE;
				ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
				ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;
				
				// Toggle the LED by doing circular addition (e.g., 0, 1, 2, 3, 0, 1...)
				GPIO_PORTN_DATA_R = (++GPIO_PORTN_DATA_R)%4;
			
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
                               (void *)(ADC0_BASE + ADC_O_SSFIFO0),
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
				
				ui32TempAvg = 0;         // clear the previous value
				// calculate the average temperature with buffer B
				// process all the DMA data
				for (int i = 0; i < ADC_BUF_SIZE; i++)
				{
					ui32TempAvg = ui32TempAvg + ui32BufB[i];
				}
				// take the average of ADC_BUF_SIZE samples over the 5 seconds
				ui32TempAvg = ui32TempAvg / ADC_BUF_SIZE;
				ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
				ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;

				// Toggle the LED by doing circular addition (e.g., 0, 1, 2, 3, 0, 1...)
				GPIO_PORTN_DATA_R = (++GPIO_PORTN_DATA_R)%4;
				
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
                               (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                               ui32BufB, ADC_BUF_SIZE);
				//Re-enable the uDMA channel											 
				uDMAChannelEnable(UDMA_CHANNEL_ADC0);
		}
}

int main(void)
{	
		unsigned int period = 4000000;
		// Turn on the LED.
	
		DMA_Init();
		ADC0_Init(199, period);
		GPIO_PORTN_DATA_R |= 0x03;
		IntMasterEnable();       		// globally enable interrupt
		ADCProcessorTrigger(ADC0_BASE, 0);
	
		while(1)
		{
			
		}
}
