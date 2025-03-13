#include "pwmgen.h"

void muxPinsTimer(const char* PwmPin, const char* DirPin)
{

}

void createPwmGen(void)
{
    const char* comment = module["Comment"];
    printf("\n%s\n", comment);

    uint8_t    joint        = module["Joint Number"];
    uint8_t    pwmNumber    = module["PWM No"];
    uint16_t   pwmFreqHz    = module["PWM Frequency"];
    uint8_t    minDutyCycle = module["Min Duty Cycle"];
    uint8_t    maxDutyCycle = module["Max Duty Cycle"];

    // configure pointers to data source and feedback location
    ptrJointFreqCmd[joint] = &rxData.jointFreqCmd[joint];
    ptrJointEnable = &rxData.jointEnable;

    // create the pwm generator, register it in the thread
    Module* pwmgen = new PwmGen(joint, pwmNumber, pwmFreqHz, minDutyCycle, maxDutyCycle,*ptrJointFreqCmd[joint], *ptrJointEnable);
    baseThread->registerModule(pwmgen);
}

PwmGen::PwmGen(uint8_t jointNumber, uint8_t pwm, uint16_t pwmFreqHz, uint8_t minDutyCycle, uint8_t maxDutyCycle, volatile int32_t &ptrFrequencyCommand, volatile uint8_t &ptrJointEnable) :
    jointNumber(jointNumber),
    pwm(pwm),
    pwmFreqHz(pwmFreqHz),
    ptrFrequencyCommand(&ptrFrequencyCommand),
    ptrJointEnable(&ptrJointEnable)
{

	/* validate duty cycle limits, both limits must be between
	   0 and 100 (inclusive) and max must be greater then min */
    this->minDutyCycle = minDutyCycle;
    this->maxDutyCycle = maxDutyCycle;
       
    if (maxDutyCycle > 100) {
	    this->maxDutyCycle = 100;
	}
	if (minDutyCycle > maxDutyCycle) {
	    this->minDutyCycle = maxDutyCycle;
        this->maxDutyCycle = maxDutyCycle;
	}
	if (minDutyCycle < 0.0) {
	    this->minDutyCycle = 0.0;
	}
	if (maxDutyCycle < minDutyCycle) {
	    this->maxDutyCycle = minDutyCycle;
        this->minDutyCycle = minDutyCycle;
	}

    this->srcClock_Hz = (CLOCK_GetFreq(kCLOCK_IpgClk) / QTMR_CLOCK_SOURCE_DIVIDER);
    this->mask = 1 << this->jointNumber;
    this->isEnabled = false;

    switch(this->pwm)
    {
        case 1:
            //PWM1 Module

            //MUX the PWM pin
            IOMUXC_SetPinMux(   IOMUXC_GPIO_AD_B1_01_TMR3_TIMER1, 0U);
            IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_01_TMR3_TIMER1, 0x10B0U);

            //Set Timer Channel
            channel = kQTMR_Channel_1;

            //Create the DIR pin
            this->dirPin = new Pin("P1_22", OUTPUT);
            break;
        case 2:
            //PWM2 Module

            //MUX the PWM pin
            IOMUXC_SetPinMux(   IOMUXC_GPIO_AD_B1_02_TMR3_TIMER2, 0U);
            IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_02_TMR3_TIMER2, 0x10B0U);

            //Set Timer Channel
            channel = kQTMR_Channel_2;

            //Create the DIR pin
            this->dirPin = new Pin("P1_25", OUTPUT);
            break;
        case 3:
            //PWM3 Module

            //MUX the PWM pin
            IOMUXC_SetPinMux(   IOMUXC_GPIO_AD_B1_00_TMR3_TIMER0, 0U);
            IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_00_TMR3_TIMER0, 0x10B0U);

            //Set Timer Channel
            channel = kQTMR_Channel_0;
            
            //Create the DIR pin
            this->dirPin = new Pin("P1_23", OUTPUT);
            break;
        case 4:
            //PWM4 Module

            //MUX the PWM pin
            IOMUXC_SetPinMux(   IOMUXC_GPIO_AD_B1_03_TMR3_TIMER3, 0U);
            IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_03_TMR3_TIMER3, 0x10B0U);

            //Set Timer Channel
            channel = kQTMR_Channel_3;
            
            //Create the DIR pin
            this->dirPin = new Pin("P1_20", OUTPUT);
            break;
        default:
            break;
    }

    //Set the Timer Configuration
    qtmr_config_t qtmrConfig;
    QTMR_GetDefaultConfig(&qtmrConfig);
    qtmrConfig.primarySource = QTMR_PRIMARY_SOURCE;
    QTMR_Init(QTMR_BASEADDR, channel, &qtmrConfig);

    printf("PWMGEN%d for JOINT%d, Created!!!!\n", this->pwm, this->jointNumber);
    printf("PWM Frequency: %d\n", this->pwmFreqHz);
    printf("Min Duty Cycle: %d\n", this->minDutyCycle);
    printf("Max Duty Cycle: %d\n", this->maxDutyCycle);
    printf("Source Clock: %lu\n", this->srcClock_Hz);
    printf("-------------------------------------\n");

}

void PwmGen::update()
{
	// Use the standard Module interface to run makePulses()

	this->makePulses();
}

void PwmGen::updatePost()
{
    this->stopPulses();
}

void PwmGen::slowUpdate()
{
    return;
}

void PwmGen::makePulses()
{

    double tmpDC = 0.0;

    this->isEnabled = ((*(this->ptrJointEnable) & this->mask) != 0);

    if (this->isEnabled == true)
    {

        if(this->frequencyCommand != *(this->ptrFrequencyCommand))
        {

            this->frequencyCommand = *(this->ptrFrequencyCommand);
        
            //Convert the frequency command to a duty cycle
            tmpDC = this->frequencyCommand;

            /* limit the duty cycle */
            if (tmpDC >= 0.0) {
                if (tmpDC > this->maxDutyCycle) {
                    tmpDC = this->maxDutyCycle;
                } else if (tmpDC < this->minDutyCycle) {
                    tmpDC = this->minDutyCycle;
                }
                this->direction = false;
                this->dutyCycle = tmpDC;
            } else {
                if (tmpDC < -this->maxDutyCycle) {
                    tmpDC = -this->maxDutyCycle;
                } else if (tmpDC > -this->minDutyCycle) {
                    tmpDC = -this->minDutyCycle;
                }
                this->direction = true;
                this->dutyCycle = -tmpDC;
            }

            // stop the timer so we can force an output state if needed
            QTMR_StopTimer(QTMR_BASEADDR, this->channel);

            // manage the Quad Timer inability to handle 0 and 100% conditions
            if (this->dutyCycle == 0.0)
            {
                // use the FORCE and VAL registers to output 0

                this->base->CHANNEL[this->channel].SCTRL = (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK | TMR_SCTRL_VAL(0));
            }
            else if (this->dutyCycle == 100.0)
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
                this->highCount   = (uint32_t) ((float)periodCount * (100-this->dutyCycle)) / 100;
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

                this->dirPin->set(this->direction);

                // restart the timer
                QTMR_StartTimer(QTMR_BASEADDR, this->channel, kQTMR_PriSrcRiseEdge);
            }
        }

    }
    else
    {
    	this->stopPulses();
    }


    return;

}


void PwmGen::stopPulses()
{
	this->dutyCycle = 0.0;
    // stop the timer so we can force an output state if needed
    QTMR_StopTimer(QTMR_BASEADDR, this->channel);

    // use the FORCE and VAL registers to output 0
    this->base->CHANNEL[this->channel].SCTRL = (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK | TMR_SCTRL_VAL(0));

    this->dirPin->set(false);
 
    return;
}


void PwmGen::setEnabled(bool state)
{
    this->isEnabled = state;
}

