#ifndef PIN_H
#define PIN_H

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>

#include "fsl_gpio.h"

#define INPUT 0x0
#define OUTPUT 0x1


class Pin
{
    private:

        std::string         portAndPin;
        uint8_t             dir;
        //uint8_t             modifier;
        uint8_t             portIndex;
        uint16_t            pinNumber;
        uint16_t            pin;
        //uint32_t            mode;
        //uint32_t            pull;
        //uint32_t            speed;
        GPIO_Type*			GPIOx;
        gpio_pin_config_t	GPIO_Config;

    public:

        Pin(std::string, int);

        void configPin();

        inline bool get()
        {
        	return 1; // change this
            //return HAL_GPIO_ReadPin(this->GPIOx, this->pin);
        }

        inline void set(bool value)
        {
            if (value)
            {
                //HAL_GPIO_WritePin(this->GPIOx, this->pin, GPIO_PIN_SET);
            }
            else
            {
                //HAL_GPIO_WritePin(this->GPIOx, this->pin, GPIO_PIN_RESET);
            }
        }
};

#endif
