#include "interrupt.h"
#include "MIMXRT1052.h"

#include <cstdio>

// Define the vector table, it is only declared in the class declaration
Interrupt* Interrupt::ISRVectorTable[] = {0};

// Constructor
Interrupt::Interrupt(void){}


// Methods

void Interrupt::Register(int interruptNumber, Interrupt* intThisPtr)
{
	printf("Registering interrupt for interrupt number = %d\n", interruptNumber);
	ISRVectorTable[interruptNumber] = intThisPtr;
}

void Interrupt::TIM1_Wrapper(void)
{
	ISRVectorTable[GPT1_IRQn]->ISR_Handler();
}

void Interrupt::TIM2_Wrapper(void)
{
	ISRVectorTable[GPT2_IRQn]->ISR_Handler();
}

void Interrupt::GPIO3_Combined_0_15_Wrapper(void)
{
	ISRVectorTable[GPIO3_Combined_0_15_IRQn]->ISR_Handler();
}

void Interrupt::GPIO3_Combined_16_31_Wrapper(void)
{
	ISRVectorTable[GPIO3_Combined_16_31_IRQn]->ISR_Handler();
}

void Interrupt::GPIO4_Combined_0_15_Wrapper(void)
{
	ISRVectorTable[GPIO4_Combined_0_15_IRQn]->ISR_Handler();
}

void Interrupt::GPIO4_Combined_16_31_Wrapper(void)
{
	ISRVectorTable[GPIO4_Combined_16_31_IRQn]->ISR_Handler();
}
