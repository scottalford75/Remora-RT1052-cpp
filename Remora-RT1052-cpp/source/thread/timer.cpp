#include <stdio.h>

#include "../interrupt/interrupt.h"
#include "timerInterrupt.h"
#include "timer.h"
#include "pruThread.h"



// Timer constructor
pruTimer::pruTimer(pit_chnl_t channel, uint32_t frequency, pruThread* ownerPtr):
	channel(channel),
	frequency(frequency),
	timerOwnerPtr(ownerPtr)
{
	interruptPtr = new TimerInterrupt(this->channel, this);	// Instantiate a new Timer Interrupt object and pass "this" pointer

	this->startTimer();
}


void pruTimer::timerTick(void)
{
	//Do something here
	this->timerOwnerPtr->run();
}



void pruTimer::startTimer(void)
{
    // The Periodic Interrupt Timer (PIT) module is used to trigger the threads

	if (this->channel == kPIT_Chnl_0)
    {
        printf("	power on Timer 0\n\r");
    }
    else if (this->channel == kPIT_Chnl_1)
    {
        printf("	power on Timer 1\n\r");
    }

    CLOCK_SetMux(kCLOCK_PerclkMux, 1U);
    CLOCK_SetDiv(kCLOCK_PerclkDiv, 0U);
    PIT_GetDefaultConfig(&this->pitConfig);
    PIT_Init(PIT, &this->pitConfig);

    PIT_SetTimerPeriod(PIT, this->channel, CLOCK_GetFreq(kCLOCK_OscClk)/this->frequency);
    PIT_EnableInterrupts(PIT, this->channel, kPIT_TimerInterruptEnable);

    EnableIRQ(PIT_IRQn);

    printf("	timer started\n");
}


void pruTimer::stopTimer()
{
	PIT_DisableInterrupts(PIT, this->channel, kPIT_TimerInterruptEnable);

    printf("	timer stop\n\r");
    PIT_StopTimer(PIT, this->channel);
}
