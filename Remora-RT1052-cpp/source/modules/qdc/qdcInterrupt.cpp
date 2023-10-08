
#include "qdcInterrupt.h"
#include "qdc.h"

QdcInterrupt::QdcInterrupt(int interruptNumber, Qdc* owner)
{
	// Allows interrupt to access owner's data
	InterruptOwnerPtr = owner;

	// When a device interrupt object is instantiated, the Register function must be called to let the
	// Interrupt base class know that there is an appropriate ISR function for the given interrupt.
	Interrupt::Register(interruptNumber, this);
}


void QdcInterrupt::ISR_Handler(void)
{
	this->InterruptOwnerPtr->indexEvent();
}
