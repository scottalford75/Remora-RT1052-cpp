#ifndef SOFTPWM_H
#define SOFTPWM_H

#include <string>

#include "../module.h"
#include "../../drivers/pin/pin.h"

class SoftPWM : public Module
{
	private:


        volatile float *ptrDuty; 			// pointer to the data source
        float 		duty;

        int	counter;
        int	period;
        int	pulse;

        std::string pwm;
        Pin* 		pwmPin;

	public:

		SoftPWM(volatile float&, std::string);

		virtual void update(void);          // Module default interface
		virtual void slowUpdate(void);      // Module default interface
};

#endif

