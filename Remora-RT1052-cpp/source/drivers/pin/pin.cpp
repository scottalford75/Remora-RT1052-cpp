#include "pin.h"
#include <cstdio>
#include <cerrno>
#include <string>


Pin::Pin(std::string portAndPin, int dir) :
    portAndPin(portAndPin),
    dir(dir)
{
    // Set direction
    if (this->dir == INPUT)
    {
		this->GPIO_Config.direction = kGPIO_DigitalInput;
		this->GPIO_Config.outputLogic = 0U;
		this->GPIO_Config.interruptMode = kGPIO_NoIntmode;
    }
    else
    {
		this->GPIO_Config.direction = kGPIO_DigitalOutput;
		this->GPIO_Config.outputLogic = 1U;
		this->GPIO_Config.interruptMode = kGPIO_NoIntmode;
    }

    this->configPin();
}



void Pin::configPin()
{
    printf("Creating Pin @\n");

    GPIO_Type* gpios[4] = {GPIO1,GPIO2,GPIO3,GPIO4};
    

    if (this->portAndPin[0] == 'P') // PX_XX e.g.P1_02 P0_15
    {  
        this->portIndex     = this->portAndPin[1] - '0';
        this->pinNumber     = this->portAndPin[3] - '0';       
        uint16_t pin2       = this->portAndPin[4] - '0';       

        if (pin2 <= 9)
        {
            this->pinNumber = this->pinNumber * 10 + pin2;
        }

        this->pin = this->pinNumber;
    }
    else
    {
        printf("  Invalid port and pin definition\n");
        return;
    }    

    printf("  port = GPIO%c\n", char('A' + this->portIndex));
    printf("  pin = %d\n", this->pinNumber);

    // translate port index into something useful
    this->GPIOx = gpios[this->portIndex];

	GPIO_PinInit(this->GPIOx, this->pin, &this->GPIO_Config);
}



