#include "DMAstepgen.h"



/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON     
************************************************************************/

void createDMAstepgen()
{
    const char* comment = module["Comment"];
    printf("\n%s\n",comment);

    int joint = module["Joint Number"];
    const char* step = module["Step Pin"];
    const char* dir = module["Direction Pin"];

    // configure pointers to data source and feedback location
    ptrJointFreqCmd[joint] = &rxData.jointFreqCmd[joint];
    ptrJointFeedback[joint] = &txData.jointFeedback[joint];
    ptrJointEnable = &rxData.jointEnable;

    // create the step generator, register it in the thread
    Module* stepgen = new DMAstepgen(DMA_FREQ, joint, step, dir, DMA_BUFFER_SIZE, STEPBIT, *ptrJointFreqCmd[joint], *ptrJointFeedback[joint], *ptrJointEnable);
    vDMAthread.push_back(stepgen);
}


/***********************************************************************
                METHOD DEFINITIONS
************************************************************************/

DMAstepgen::DMAstepgen(int32_t threadFreq, int jointNumber, std::string step, std::string direction, int DMAbufferSize, int stepBit, volatile int32_t &ptrFrequencyCommand, volatile int32_t &ptrFeedback, volatile uint8_t &ptrJointEnable) :
	jointNumber(jointNumber),
	step(step),
	direction(direction),
	DMAbufferSize(DMAbufferSize),
	stepBit(stepBit),
	ptrFrequencyCommand(&ptrFrequencyCommand),
	ptrFeedback(&ptrFeedback),
	ptrJointEnable(&ptrJointEnable),
	stepTime(1),
	dirHoldTime(1),
	dirSetupTime(1)
{
	uint8_t pin, pin2;

	stepDMAbuffer_0 = &stepgenDMAbuffer_0[0];
	stepDMAbuffer_1 = &stepgenDMAbuffer_1[0];
	stepDMAactiveBuffer = &stepgenDMAbuffer;
	this->frequencyCommand = 500000;


	dirDMAbuffer_0 = &stepgenDMAbuffer_0[0];
	dirDMAbuffer_1 = &stepgenDMAbuffer_1[0];
	dirDMAactiveBuffer = &stepgenDMAbuffer;


	this->stepPin = new Pin(this->step, OUTPUT);
	this->directionPin = new Pin(this->direction, OUTPUT);
	//this->debug = new Pin("PE_10", OUTPUT);
	this->accumulator = 0;
	this->remainder = 0;
	//this->prevRemainder = BUFFER_COUNTS;
	this->prevRemainder = 0;
	this->stepLow = 0;
	this->mask = 1 << this->jointNumber;
	this->isEnabled = false;
	//this->isForward = false;

	// determine the step pin number from the portAndPin string
	pin = this->step[3] - '0';
	pin2 = this->step[4] - '0';
	if (pin2 <= 9) pin = pin * 10 + pin2;
	this->stepMask = 1 << pin;

	// determine the dir pin number from the portAndPin string
	pin = this->direction[3] - '0';
	pin2 = this->direction[4] - '0';
	if (pin2 <= 8) pin = pin * 10 + pin2;
	this->dirMask = 1 << pin;

	this->isEnabled = true; // TESTING!!! to be removed
}


void DMAstepgen::update()
{
	// Use the standard Module interface to run makePulses()
	this->makePulses();
}

void DMAstepgen::updatePost()
{
	this->stopPulses();
}

void DMAstepgen::slowUpdate()
{
	return;
}

void DMAstepgen::makePulses()
{
	//this->isEnabled = ((*(this->ptrJointEnable) & this->mask) != 0);

	if (this->isEnabled == true)  												// this Step generator is enables so make the pulses
	{
		this->oldaddValue = this->addValue;
		//this->frequencyCommand = *(this->ptrFrequencyCommand);            		// Get the latest frequency command via pointer to the data source
		this->addValue = (BUFFER_COUNTS * PRU_SERVOFREQ) / abs(this->frequencyCommand);		// determine the add value from the commanded frequency ratio

		// determine which double buffer to fill
		if (!*stepDMAactiveBuffer)	// false = buffer_0
		{
			stepDMAbuffer = stepDMAbuffer_0;
		}
		else // buffer_1
		{
			stepDMAbuffer = stepDMAbuffer_1;
		}

		// finish the step from the previous period if needed
		if (this->stepLow != 0)
		{
			// put step low into DMA buffer
			*(stepDMAbuffer + this->stepLow - 1) |= this->stepMask;
			this->stepLow = 0;

		}

		if (this->addValue <= BUFFER_COUNTS)
		{
			//printf("stepping...\n\r");

			// 1 or more steps in this servo period

			this->remainder = BUFFER_COUNTS - this->prevRemainder;

			//this->debug->set(1);

			while(this->remainder >= this->addValue)
			{
				// we can still step in this servo period
				this->accumulator = this->accumulator + this->addValue;
				this->remainder = BUFFER_COUNTS - this->accumulator;
				this->stepPos = this->accumulator / (RESOLUTION / 2);

				// map stepPos (1 - 500) to DMA buffer (0 - 999)

				this->stepHigh = this->stepPos;// - 2;
				this->stepLow = this->stepHigh + 1;

				//printf("acc = %ld, rem = %ld, stepH = %d, stepL = %d\n\r", this->accumulator, this->remainder, this->stepHigh, this->stepLow);

				// put step high into DMA buffer
				*(stepDMAbuffer + this->stepHigh) |= this->stepMask;
				this->stepHigh = 0;

				// put step low into DMA buffer
				// step low could be in the next period (buffer)
				if (this->stepLow <= DMA_BUFFER_SIZE - 1)
				{
					// put step low into DMA buffer
					*(stepDMAbuffer + this->stepLow) |= this->stepMask;
					this->stepLow = 0;
				}
				else
				{
					this->stepLow = this->stepLow - DMA_BUFFER_SIZE;
				}

			}

			//this->debug->set(0);

			// reset accumulator and carry remainder into the next servo period
			this->accumulator = 0;
			this->prevRemainder = this->remainder;
		}
		else
		{
			// 1 or no steps in this servo period

		}
	}
}


void DMAstepgen::stopPulses()
{

}


void DMAstepgen::setEnabled(bool state)
{
	this->isEnabled = state;
}
