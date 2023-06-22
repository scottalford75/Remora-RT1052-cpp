#include "interrupt.h"

#include "fsl_pit.h"

extern "C" {

void PIT_IRQHandler(void)
{
	volatile uint32_t status;

    status = PIT_GetStatusFlags(PIT, kPIT_Chnl_0);
	if(status & (uint32_t)kPIT_TimerFlag)
	{
		PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
		__DSB();

		Interrupt::PIT_Chn0_Wrapper();
	}

    status = PIT_GetStatusFlags(PIT, kPIT_Chnl_1);
	if(status & (uint32_t)kPIT_TimerFlag)
	{
		PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
		__DSB();

		Interrupt::PIT_Chn1_Wrapper();
	}

}

}
