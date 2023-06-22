#ifndef TIMER_H
#define TIMER_H

#include "fsl_pit.h"
#include <stdint.h>

class TimerInterrupt; // forward declaration
class pruThread; // forward declaration

class pruTimer
{
	friend class TimerInterrupt;

	private:

		TimerInterrupt* 	interruptPtr;
		pit_chnl_t 			channel;
		uint32_t 			frequency;
		pruThread* 			timerOwnerPtr;

		pit_config_t 		pitConfig;

		void startTimer(void);
		void timerTick();			// Private timer tiggered method

	public:

		pruTimer(pit_chnl_t channel, uint32_t frequency, pruThread* ownerPtr);
        void stopTimer(void);

};

#endif
