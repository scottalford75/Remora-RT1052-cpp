
#include <modules/qdc/portInterrupt.h>
#include "qdc.h"

portInterrupt* portInterrupt::IndexPinISRVectorTable[PORT_COMBINED_IRQn][INDEX_PIN_COUNT_IRQn]={0};

portInterrupt::portInterrupt(Qdc* owner):InterruptOwnerPtr(owner)
{
	// Allows interrupt to access owner's data
	//InterruptOwnerPtr = owner;

    gpio_pin_config_t pin_config = {
        kGPIO_DigitalInput,
        0,
        kGPIO_IntRisingEdge,
    };

	// When a device interrupt object is instantiated, the Register function must be called to let the
	// Interrupt base class know that there is an appropriate ISR function for the given interrupt.
    this->Register(this->InterruptOwnerPtr->irq,this);
	EnableIRQ(this->InterruptOwnerPtr->irq);

	GPIO_PinInit(this->InterruptOwnerPtr->gpioBase, this->InterruptOwnerPtr->indexPinInNumber, &pin_config);
    GPIO_PortEnableInterrupts(this->InterruptOwnerPtr->gpioBase, 1U << this->InterruptOwnerPtr->indexPinInNumber);
}

void portInterrupt::Register(IRQn_Type irq,portInterrupt* intThisPtr)
{
	printf("Registering interrupt for interrupt number = %d\n", irq);

	for(uint8_t i=0;i<INDEX_PIN_COUNT_IRQn;i++)
	{
		if(!IndexPinISRVectorTable[irq & 0x03][i])
		{
			IndexPinISRVectorTable[irq & 0x03][i] = intThisPtr;
			break;
		}
	}

}

void portInterrupt::GPIO34_Combined_Wrapper(IRQn_Type combinedIrq)
{
	for(uint8_t i=0;i<INDEX_PIN_COUNT_IRQn;i++)
	{
		if(IndexPinISRVectorTable[combinedIrq & 0x03][i])
		{
			if(GPIO_PortGetInterruptFlags(IndexPinISRVectorTable[combinedIrq & 0x03][i]->InterruptOwnerPtr->gpioBase) & 1 << IndexPinISRVectorTable[combinedIrq & 0x03][i]->InterruptOwnerPtr->indexPinInNumber)
			{
				IndexPinISRVectorTable[combinedIrq & 0x03][i]->ISR_Handler();
				GPIO_PortClearInterruptFlags(IndexPinISRVectorTable[combinedIrq & 0x03][i]->InterruptOwnerPtr->gpioBase, 1U << IndexPinISRVectorTable[combinedIrq & 0x03][i]->InterruptOwnerPtr->indexPinInNumber);
			}
		}
		else
		{
			break;
		}
	}
}

void portInterrupt::ISR_Handler(void)
{
	this->InterruptOwnerPtr->indexEvent();
}

