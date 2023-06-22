#ifndef INTERRUPT_H
#define INTERRUPT_H

// Base class for all interrupt derived classes

#define PERIPH_COUNT_IRQn	4				// 4 PIT timers available

class Interrupt
{
	protected:

		static Interrupt* ISRVectorTable[PERIPH_COUNT_IRQn];

	public:

		Interrupt(void);

		static void Register(int interruptNumber, Interrupt* intThisPtr);

		// wrapper functions to ISR_Handler()
		static void PIT_Chn0_Wrapper();
        static void PIT_Chn1_Wrapper();

		virtual void ISR_Handler(void) = 0;

};

#endif
