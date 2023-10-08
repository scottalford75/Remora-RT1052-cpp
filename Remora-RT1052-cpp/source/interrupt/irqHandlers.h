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

	void ENC1_IRQHandler()
	{
		if((ENC_GetStatusFlags(ENC1) & kENC_INDEXPulseFlag))
		{
			ENC_ClearStatusFlags(ENC1, kENC_INDEXPulseFlag);
		}
		__DSB();
	}

	void ENC2_IRQHandler()
	{
		if((ENC_GetStatusFlags(ENC2) & kENC_INDEXPulseFlag))
		{
			ENC_ClearStatusFlags(ENC2, kENC_INDEXPulseFlag);
			Interrupt::ENC2_Wrapper();
		}
		__DSB();
	}

	void ENC3_IRQHandler()
	{
		if((ENC_GetStatusFlags(ENC3) & kENC_INDEXPulseFlag))
		{
			ENC_ClearStatusFlags(ENC3, kENC_INDEXPulseFlag);
			Interrupt::ENC3_Wrapper();
		}
		__DSB();
	}

}
