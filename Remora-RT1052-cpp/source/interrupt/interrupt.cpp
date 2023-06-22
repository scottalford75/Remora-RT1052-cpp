#include "interrupt.h"

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

void Interrupt::PIT_Chn0_Wrapper(void)
{
	ISRVectorTable[0]->ISR_Handler();
}

void Interrupt::PIT_Chn1_Wrapper(void)
{
	ISRVectorTable[1]->ISR_Handler();
}

