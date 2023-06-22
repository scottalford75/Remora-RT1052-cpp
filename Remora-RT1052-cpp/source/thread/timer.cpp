#include <stdio.h>

#include "../interrupt/interrupt.h"
#include "timerInterrupt.h"
#include "timer.h"
#include "pruThread.h"

#include "fsl_gpt.h"

// Timer constructor
pruTimer::pruTimer(GPT_Type* timer, IRQn_Type irq, uint32_t frequency, pruThread* ownerPtr):
	timer(timer),
	irq(irq),
	frequency(frequency),
	timerOwnerPtr(ownerPtr)
{
	interruptPtr = new TimerInterrupt(this->irq, this);	// Instantiate a new Timer Interrupt object and pass "this" pointer

	this->startTimer();
}


void pruTimer::timerTick(void)
{
	//Do something here
	this->timerOwnerPtr->run();
}



void pruTimer::startTimer(void)
{
	uint32_t gptFreq, compareVal;
	gpt_config_t gptConfig;

    if (this->timer == GPT1)
    {
        printf("	configuring Timer 1\n\r");
    }
    else if (this->timer == GPT2)
    {
        printf("	configuring Timer 2\n\r");
    }


    gptFreq = CLOCK_GetFreq(kCLOCK_PerClk);

    //timer update frequency = TIM_CLK/(PSC+1)/TIM_ARR

    compareVal = gptFreq / this->frequency;

    GPT_GetDefaultConfig(&gptConfig);
    GPT_Init(this->timer, &gptConfig);
    GPT_SetOutputCompareValue(this->timer, kGPT_OutputCompare_Channel1, compareVal);
    GPT_EnableInterrupts(this->timer, kGPT_OutputCompare1InterruptEnable);
    EnableIRQ(this->irq);
    GPT_StartTimer(this->timer);

    printf("	timer started\n");
}

void pruTimer::stopTimer()
{
    DisableIRQ(this->irq);

    printf("	timer stop\n\r");
    GPT_StopTimer(this->timer);
}
