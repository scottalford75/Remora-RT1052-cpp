#include "fsl_gpt.h"
#include "interrupt.h"
#include "../modules/qdc/portInterrupt.h"

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

	//GPIO_Combined_IRQHandlers
	void GPIO3_Combined_0_15_IRQHandler()
	{
		portInterrupt::GPIO34_Combined_Wrapper(GPIO3_Combined_0_15_IRQn);
		__DSB();
	}

	void GPIO3_Combined_16_31_IRQHandler()
	{
		portInterrupt::GPIO34_Combined_Wrapper(GPIO3_Combined_16_31_IRQn);
		__DSB();
	}

	void GPIO4_Combined_0_15_IRQHandler()
	{
		portInterrupt::GPIO34_Combined_Wrapper(GPIO4_Combined_0_15_IRQn);
		__DSB();
	}

	void GPIO4_Combined_16_31_IRQHandler()
	{
		portInterrupt::GPIO34_Combined_Wrapper(GPIO4_Combined_16_31_IRQn);
		__DSB();
	}

}
