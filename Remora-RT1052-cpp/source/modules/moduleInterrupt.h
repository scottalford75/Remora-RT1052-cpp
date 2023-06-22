#ifndef MODULEINTERRUPT_H
#define MODULEINTERRUPT_H

#include "../interrupt/interrupt.h"
#include "module.h"

// Derived class for module interrupts


class ModuleInterrupt : public Interrupt
{
	private:
	    
		Module* InterruptOwnerPtr;
	
	public:

		ModuleInterrupt(int interruptNumber, Module* ownerptr);
    
		void ISR_Handler(void);
};

#endif
