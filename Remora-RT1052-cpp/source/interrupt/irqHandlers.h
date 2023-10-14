#include "fsl_gpt.h"
#include "fsl_enc.h"
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

	void GPIO3_Combined_0_15_IRQHandler()
	{
		Interrupt::GPIO3_Combined_0_15_Wrapper();
		__DSB();
	}

	void GPIO3_Combined_16_31_IRQHandler()
	{
		Interrupt::GPIO3_Combined_16_31_Wrapper();
		__DSB();
	}

	void GPIO4_Combined_0_15_IRQHandler()
	{
		Interrupt::GPIO4_Combined_0_15_Wrapper();
		__DSB();
	}

	void GPIO4_Combined_16_31_IRQHandler()
	{
		Interrupt::GPIO4_Combined_16_31_Wrapper();
		__DSB();
	}

}
