#ifndef TIMER_H
#define TIMER_H

#include "MIMXRT1052.h"
#include <stdint.h>

#include "fsl_gpt.h"

//#define TIM_PSC 1
//#define APB1CLK SystemCoreClock/2
//#define APB2CLK SystemCoreClock

class TimerInterrupt; // forward declaration
class pruThread; // forward declaration

class pruTimer
{
	friend class TimerInterrupt;

	private:

		TimerInterrupt* 	interruptPtr;
		GPT_Type* 	    	timer;
		IRQn_Type 			irq;
		uint32_t 			frequency;
		pruThread* 			timerOwnerPtr;

		void startTimer(void);
		void timerTick();			// Private timer triggered method

	public:

		pruTimer(GPT_Type* timer, IRQn_Type irq, uint32_t frequency, pruThread* ownerPtr);
        void stopTimer(void);

};

#endif
