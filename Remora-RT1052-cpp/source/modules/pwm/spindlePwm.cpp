#include "spindlePwm.h"

// Module for RT1052 spindle RPM on pin GPIO1_IO08

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON
************************************************************************/

void createSpindlePWM()
{
    const char* comment = module["Comment"];
    printf("\n%s\n",comment);

    int sp = module["SP[i]"];

    ptrSetPoint[sp] = &rxData.setPoint[sp];
    Module* spindle = new SpindlePWM(*ptrSetPoint[sp]);
    servoThread->registerModule(spindle);
}


/***********************************************************************
                METHOD DEFINITIONS
************************************************************************/

SpindlePWM::SpindlePWM(volatile float &ptrPwmPulseWidth) :
    ptrPwmPulseWidth(&ptrPwmPulseWidth)
{
	// VSD PWM -> Analog 0-10V
	IOMUXC_SetPinMux(IOMUXC_GPIO_B0_00_TMR1_TIMER0, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_B0_00_TMR1_TIMER0, 0x10B0U);

    qtmr_config_t qtmrConfig;

    QTMR_GetDefaultConfig(&qtmrConfig);
    qtmrConfig.primarySource = QTMR_PRIMARY_SOURCE;
    QTMR_Init(QTMR_BASEADDR, QTMR_PWM_CHANNEL, &qtmrConfig);
}



void SpindlePWM::update()
{

    if (*(this->ptrPwmPulseWidth) != this->pwmPulseWidth)
    {
        // PWM duty has changed
        this->pwmPulseWidth = *(this->ptrPwmPulseWidth);

        if (this->pwmPulseWidth > 100.0)
        {
        	this->pwmPulseWidth = 100.0;
        }

        if (this->pwmPulseWidth < 0.0)
        {
        	this->pwmPulseWidth = 0.0;
        }

        // stop the timer so we can force an output state if needed
        QTMR_StopTimer(QTMR_BASEADDR, QTMR_PWM_CHANNEL);

        // manage the Quad Timer inability to handle 0 and 100% conditions
        if (this->pwmPulseWidth == 0.0)
        {
        	// use the FORCE and VAL registers to output 0
        	this->base->CHANNEL[this->channel].SCTRL = (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK | TMR_SCTRL_VAL(0));
        }
        else if (this->pwmPulseWidth == 100.0)
        {
        	// use the FORCE and VAL registers to output 1
        	this->base->CHANNEL[this->channel].SCTRL = (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK | TMR_SCTRL_VAL(1));
        }
        else
        {
            /* Set OFLAG pin for output mode and force out a low on the pin */
            base->CHANNEL[this->channel].SCTRL |= (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK);

            /* Counter values to generate a PWM signal */
            this->periodCount = srcClock_Hz / pwmFreqHz;
            this->highCount   = (uint32_t) ((float)periodCount * this->pwmPulseWidth) / 100;
            this->lowCount    = this->periodCount - this->highCount;

            if (this->highCount > 0U)
            {
            	this->highCount -= 1U;
            }
            if (this->lowCount > 0U)
            {
            	this->lowCount -= 1U;
            }

            /* Setup the compare registers for PWM output */
            this->base->CHANNEL[this->channel].COMP1 = (uint16_t)this->lowCount;
            this->base->CHANNEL[this->channel].COMP2 = (uint16_t)this->highCount;

            /* Setup the pre-load registers for PWM output */
            this->base->CHANNEL[this->channel].CMPLD1 = (uint16_t)this->lowCount;
            this->base->CHANNEL[this->channel].CMPLD2 = (uint16_t)this->highCount;

            this->reg = base->CHANNEL[this->channel].CSCTRL;
            /* Setup the compare load control for COMP1 and COMP2.
             * Load COMP1 when CSCTRL[TCF2] is asserted, load COMP2 when CSCTRL[TCF1] is asserted
             */
            this->reg &= (uint16_t)(~(TMR_CSCTRL_CL1_MASK | TMR_CSCTRL_CL2_MASK));
            this->reg |= (TMR_CSCTRL_CL1(kQTMR_LoadOnComp2) | TMR_CSCTRL_CL2(kQTMR_LoadOnComp1));
            this->base->CHANNEL[this->channel].CSCTRL = reg;

            if (this->outputPolarity)
            {
                /* Invert the polarity */
            	this->base->CHANNEL[this->channel].SCTRL |= TMR_SCTRL_OPS_MASK;
            }
            else
            {
                /* True polarity, no inversion */
            	this->base->CHANNEL[this->channel].SCTRL &= ~(uint16_t)TMR_SCTRL_OPS_MASK;
            }

            this->reg =this-> base->CHANNEL[this->channel].CTRL;
            this->reg &= ~(uint16_t)TMR_CTRL_OUTMODE_MASK;
            /* Count until compare value is  reached and re-initialize the counter, toggle OFLAG output
             * using alternating compare register
             */
            this->reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_ToggleOnAltCompareReg));
            this->base->CHANNEL[this->channel].CTRL = this->reg;

            // restart the timer
            QTMR_StartTimer(QTMR_BASEADDR, QTMR_PWM_CHANNEL, kQTMR_PriSrcRiseEdge);
        }
    }

    return;
}


void SpindlePWM::slowUpdate()
{
	return;
}
