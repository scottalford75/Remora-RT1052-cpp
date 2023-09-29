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
    	this->config.direction = kGPIO_DigitalInput;
    	this->config.outputLogic = 1U;
    	this->config.interruptMode = kGPIO_NoIntmode;
    }
    else if (this->dir == OUTPUT)
    {
    	this->config.direction = kGPIO_DigitalOutput;
    	this->config.outputLogic = 0U;
    	this->config.interruptMode = kGPIO_NoIntmode;
    }


    printf("Creating Pin @\n");
    
    if (this->portAndPin[0] == 'P') // PX_XX e.g.P0_2 P1_15
    {  
        this->port     	= this->portAndPin[1] - '0';
        this->pin    	= this->portAndPin[3] - '0';
        uint16_t pin2   = this->portAndPin[4] - '0';

        if (pin2 <= 9)
        {
            this->pin = this->pin * 10 + pin2;
        }
    }
    else
    {
        printf("  Invalid port and pin definition\n");
        return;
    }    

    printf("  port = GPIO%d\n", this->port);
    printf("  pin = %d\n", this->pin);

    if (this->port == 1)
    {
    	this->GPIOx = GPIO1;
    }
    else if (this->port == 2)
    {
    	this->GPIOx = GPIO2;
    }
    else if (this->port == 3)
    {
    	this->GPIOx = GPIO3;
    }
    else if (this->port == 4)
    {
    	this->GPIOx = GPIO4;
    }

    GPIO_PinInit(this->GPIOx, this->pin, &this->config);
}


