#ifndef QDCINTERRUPT_H
#define QDCINTERRUPT_H
// Derived class for timer interrupts
#include "../../interrupt/interrupt.h"
class Qdc; // forward declaration

class QdcInterrupt : public Interrupt
{
	private:

		Qdc* InterruptOwnerPtr;

	public:

		QdcInterrupt(int , Qdc*);

		void ISR_Handler(void);
};

#endif
