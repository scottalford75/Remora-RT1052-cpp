#ifndef SPINDLEPWM_H
#define SPINDLEPWM_H

#include "fsl_iomuxc.h"
#include "fsl_qtmr.h"

#include "../module.h"

#include "extern.h"

#define QTMR_BASEADDR              	TMR1
#define QTMR_PWM_CHANNEL           	kQTMR_Channel_0
#define QTMR_PRIMARY_SOURCE       	(kQTMR_ClockDivide_32)
#define QTMR_CLOCK_SOURCE_DIVIDER 	(32U)
#define PWM_FREQ					100

void createSpindlePWM(void);

class SpindlePWM : public Module
{
	private:

        volatile float*				ptrPwmPulseWidth; 	// pointer to the data source
        float 						pwmPulseWidth;                // Pulse width (%)

    	TMR_Type*					base = QTMR_BASEADDR;
    	qtmr_channel_selection_t 	channel = QTMR_PWM_CHANNEL;
    	uint32_t 					pwmFreqHz = PWM_FREQ;
    	bool 						outputPolarity = false;
    	uint32_t 					srcClock_Hz = (CLOCK_GetFreq(kCLOCK_IpgClk) / QTMR_CLOCK_SOURCE_DIVIDER);

        uint32_t 					periodCount;
        uint32_t					highCount;
		uint32_t 					lowCount;
        uint16_t 					reg;

	public:

		SpindlePWM(volatile float&);

		virtual void update(void);          // Module default interface
		virtual void slowUpdate(void);      // Module default interface
};

#endif

