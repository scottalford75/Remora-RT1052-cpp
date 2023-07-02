#include "softPwm.h"



SoftPWM::SoftPWM(volatile float &ptrDuty, std::string pwm) :
    ptrDuty(&ptrDuty),
	pwm(pwm)
{
	this->pwmPin = new Pin(this->pwm, OUTPUT);
	this->counter = 0;
	this->period = 1000;
}



void SoftPWM::update()
{
    if (*(this->ptrDuty) != this->duty)
    {
        // PWM duty has changed
        this->duty = *(this->ptrDuty);

        if (this->duty > 100.0)
        {
        	this->duty = 100.0;
        }

        if (this->duty < 0.0)
        {
        	this->duty = 0.0;
        }

        this->pulse = (float)this->period*(this->duty / 100.0);
    }

	if (this->counter == this->pulse) this->pwmPin->set(false);

	this->counter++;

	if (this->counter == this->period)
	{
		this->pwmPin->set(true);
		this->counter = 0;
	}

    return;
}


void SoftPWM::slowUpdate()
{
	return;
}
