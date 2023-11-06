#ifndef INTERRUPT_H
#define INTERRUPT_H

// Base class for all interrupt derived classes

#define PERIPH_COUNT_IRQn	152				// Total number of device interrupt sources

class Interrupt
{
	protected:

		static Interrupt* ISRVectorTable[PERIPH_COUNT_IRQn];

	public:

		Interrupt(void);

		static void Register(int interruptNumber, Interrupt* intThisPtr);

		// wrapper functions to ISR_Handler()
		static void TIM1_Wrapper();
        static void TIM2_Wrapper();

		virtual void ISR_Handler(void) = 0;

};

#endif
