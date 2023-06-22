#ifndef PIN_H
#define PIN_H

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>

#include "MIMXRT1052.h"
#include "fsl_gpio.h"

#define INPUT 0x0
#define OUTPUT 0x1

#define NONE        0b000
#define OPENDRAIN   0b001
#define PULLUP      0b010
#define PULLDOWN    0b011
#define PULLNONE    0b100

class Pin
{
    private:

        std::string         portAndPin;
        uint8_t             dir;
        uint8_t             port;
        uint16_t            pin;
        gpio_pin_config_t   config;
        GPIO_Type *			GPIOx;

    public:

        Pin(std::string, int);

        inline bool get()
        {
            return GPIO_PinRead(this->GPIOx, this->pin);
        }

        inline void set(bool value)
        {
        	GPIO_PinWrite(this->GPIOx, this->pin, value);
        }
};

#endif
