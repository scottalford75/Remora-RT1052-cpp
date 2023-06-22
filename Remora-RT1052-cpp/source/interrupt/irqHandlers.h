#include "fsl_gpt.h"
#include "interrupt.h"


extern "C" {

	void GPT1_IRQHandler()
	{
		if(GPT_GetStatusFlags(GPT1, kGPT_OutputCompare1Flag))
		{
			GPT_ClearStatusFlags(GPT1, kGPT_OutputCompare1Flag);
			Interrupt::TIM1_Wrapper();
		}
		__DSB();
	}

	void GPT2_IRQHandler()
	{
		if(GPT_GetStatusFlags(GPT2, kGPT_OutputCompare1Flag))
		{
			GPT_ClearStatusFlags(GPT2, kGPT_OutputCompare1Flag);
			Interrupt::TIM2_Wrapper();
		}
		__DSB();
	}

}
