#ifndef QDCINTERRUPT_H
#define QDCINTERRUPT_H
// Derived class for timer interrupts
#include "../../interrupt/interrupt.h"
#include "fsl_gpio.h"

class Qdc; // forward declaration

#define PORT_COMBINED_IRQn 4U
#define INDEX_PIN_COUNT_IRQn 4U

class portInterrupt: public Interrupt
{
	protected:

		static portInterrupt* IndexPinISRVectorTable[PORT_COMBINED_IRQn][INDEX_PIN_COUNT_IRQn];

	private:

		Qdc* InterruptOwnerPtr;

        static void Register(IRQn_Type , portInterrupt*);


	public:

		portInterrupt(Qdc*);

		static void GPIO34_Combined_Wrapper(IRQn_Type);

		void ISR_Handler(void);
};

#endif
