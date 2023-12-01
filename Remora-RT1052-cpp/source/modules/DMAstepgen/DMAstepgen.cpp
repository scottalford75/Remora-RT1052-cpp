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
    int stepLength = module["Step Length"];
    int stepSpace = module["Step Space"];
    int dirHold = module["Dir Hold"];
    int dirSetup = module["Dir Setup"];

    // configure pointers to data source and feedback location
    ptrJointFreqCmd[joint] = &rxData.jointFreqCmd[joint];
    ptrJointFeedback[joint] = &txData.jointFeedback[joint];
    ptrJointEnable = &rxData.jointEnable;

    // create the step generator, register it in the thread
    Module* stepgen = new DMAstepgen(DMA_FREQ, joint, step, dir, DMA_BUFFER_SIZE, STEPBIT, *ptrJointFreqCmd[joint], *ptrJointFeedback[joint], *ptrJointEnable, stepLength, stepSpace, dirHold, dirSetup);
    dmaThread->registerModule(stepgen);
}


/***********************************************************************
                METHOD DEFINITIONS
************************************************************************/

DMAstepgen::DMAstepgen(int32_t threadFreq, int jointNumber, std::string step, std::string direction, int DMAbufferSize, int stepBit, volatile int32_t &ptrFrequencyCommand, volatile int32_t &ptrFeedback, volatile uint8_t &ptrJointEnable, uint8_t stepLength, uint8_t stepSpace, uint8_t dirHold, uint8_t dirSetup) :
	jointNumber(jointNumber),
	step(step),
	direction(direction),
	DMAbufferSize(DMAbufferSize),
	stepBit(stepBit),
	ptrFrequencyCommand(&ptrFrequencyCommand),
	ptrFeedback(&ptrFeedback),
	ptrJointEnable(&ptrJointEnable),
	stepLength(stepLength),
	stepSpace(stepSpace),
	dirHold(dirHold),
	dirSetup(dirSetup)
{
	uint8_t pin, pin2;

	stepDMAbuffer_0 = &stepgenDMAbuffer_0[0];
	stepDMAbuffer_1 = &stepgenDMAbuffer_1[0];
	stepDMAactiveBuffer = &stepgenDMAbuffer;

	dirDMAbuffer_0 = &stepgenDMAbuffer_0[0];
	dirDMAbuffer_1 = &stepgenDMAbuffer_1[0];
	dirDMAactiveBuffer = &stepgenDMAbuffer;


	this->stepPin = new Pin(this->step, OUTPUT);
	this->directionPin = new Pin(this->direction, OUTPUT);
	this->accumulator = 0;
	this->remainder = 0;
	this->stepLow = 0;
	this->mask = 1 << this->jointNumber;
	this->isEnabled = false;
	this->dir = false;

	if (this->stepLength == 0) this->stepLength = 1;
	if (this->stepSpace == 0) this->stepSpace = 1;
	if (this->dirHold == 0) this->dirHold = 1;
	if (this->dirSetup == 0) this->dirSetup = 1;

	this->minAddValue = (this->stepLength + this->stepSpace) * (RESOLUTION / 2);

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
	/*
	(1) steplen
	(2) stepspace
	(3) dirhold
	(4) dirsetup

			   _____         _____               _____
	STEP  ____/     \_______/     \_____________/     \______
			  |     |       |     |             |     |
	Time      |-(1)-|--(2)--|-(1)-|--(3)--|-(4)-|-(1)-|
										  |__________________
	DIR   ________________________________/

	 */


	this->isEnabled = ((*(this->ptrJointEnable) & this->mask) != 0);

	if (this->isEnabled)  									// this Step generator is enabled so make the pulses
	{
		this->frequencyCommand = *(this->ptrFrequencyCommand);            		// Get the latest frequency command via pointer to the data source
		if (this->frequencyCommand != 0)
		{
			this->oldaddValue = this->addValue;

			this->addValue = (BUFFER_COUNTS * PRU_SERVOFREQ) / abs(this->frequencyCommand);		// determine the add value from the commanded frequency ratio

			if (this->addValue < this->minAddValue)
			{
				this->addValue = this->minAddValue;			// limit the frequency to step requirements
			}

			// determine which ping-pong buffer to fill
			if (!*stepDMAactiveBuffer)	// false = buffer_0
			{
				stepDMAbuffer = stepDMAbuffer_0;
			}
			else // buffer_1
			{
				stepDMAbuffer = stepDMAbuffer_1;
			}

			// get the current dir output
			this->oldDir = this->directionPin->get();

			// what's the direction for this period
			if (this->frequencyCommand < 0)
			{
				this->dir = false; // backwards
			}
			else
			{
				this->dir = true; // forwards
			}

			// change of direction?
			if (this->dir != this->oldDir)
			{
				this->dirChange = true;
			}

			if (this->dirChange)
			{
				if (this->isStepping)
				{
					this->dirPos = this->stepLow + this->dirHold;
				}
				else if (this->prevRemainder < this->dirHold)
				{
					this->dirPos = this->dirHold - this->prevRemainder;
				}
				else
				{
					this->dirPos = 0;
				}

				// toggle the direction pin
				*(stepDMAbuffer + this->dirPos) |= this->dirMask;
			}

			// finish the step from the previous period if needed
			if (this->isStepping)
			{
				// put step low into DMA buffer
				*(stepDMAbuffer + this->stepLow) |= this->stepMask;
				this->stepLow = 0;
				this->isStepping = false;
			}

			// accumulator cannot go negative, so keep prevRemainder within limits
			if (this->prevRemainder > this->addValue)
			{
				this->prevRemainder = this->addValue;
			}

			if (this->addValue - this->prevRemainder <= BUFFER_COUNTS)
			{
				// at least one step in this period
				this->accumulator = this->addValue - this->prevRemainder;

				if (this->dirChange)
				{
					this->stepPos = (this->dirPos + this->dirSetup)*(RESOLUTION/2);
					if (this->accumulator < this->stepPos)
					{
						this->accumulator = this->stepPos;
					}
				}

				this->remainder = BUFFER_COUNTS - this->accumulator;

				// ensure we are within the buffer size
				if (this-> remainder == 0)
				{
					this->accumulator--;
				}

				this->makeStep();

				while (this->remainder >= this->addValue)
				{
					// we can still step in this period
					this->accumulator = this->accumulator + this->addValue;
					this->remainder = BUFFER_COUNTS - this->accumulator;
					if (this-> remainder == 0)
					{
						this->accumulator--;
					}
					this->makeStep();
				}

				// reset accumulator and carry remainder into the next period
				this->accumulator = 0;
				this->prevRemainder = this->remainder;
				this->dirChange = false;

				// update DDS accumulator (for compatibility with software stepgen)
				this->DDSaccumulator = this->rawCount << this->stepBit;
				//*(this->ptrFeedback) = this->DDSaccumulator;                     // Update position feedback via pointer to the data receiver
				*(this->ptrFeedback) = this->rawCount;                     // Update position feedback via pointer to the data receiver
			}
			else
			{
				this->prevRemainder = this->prevRemainder + BUFFER_COUNTS;
				this->dirChange = false;
			}
		}
	}
	else
	{
		// ensure the pin is in a know state as we're using DR_TOGGLE
		this->stepPin->set(0);
		this->directionPin->set(0);
		this->dir = true;
		this->prevRemainder = 0;
		this->isStepping = false;
	}
}


void DMAstepgen::makeStep()
{
	// map stepPos (1 - 500) to DMA buffer (0 - 999)
	this->stepPos = this->accumulator / (RESOLUTION / 2);
	this->stepHigh = this->stepPos;

	// respect direction setup time
	if (this->dirChange && this->stepHigh == 0)
	{
		this->stepHigh = this->dirSetup;
	}

	this->stepLow = this->stepHigh + this->stepLength;

	// put step high into DMA buffer
	*(stepDMAbuffer + this->stepHigh) |= this->stepMask;
	this->stepHigh = 0;
	this->isStepping = true;

	// update the raw step count
	if (this->dir)
	{
		this->rawCount++;
	}
	else
	{
		this->rawCount--;
	}

	// put step low into DMA buffer
	// step low could be in the next period (buffer)
	if (this->stepLow <= DMA_BUFFER_SIZE - 1)
	{
		// put step low into DMA buffer
		*(stepDMAbuffer + this->stepLow) |= this->stepMask;
		this->stepLow = 0;
		this->isStepping = false;
	}
	else
	{
		this->stepLow = this->stepLow - DMA_BUFFER_SIZE;
	}
}


void DMAstepgen::stopPulses()
{

}


void DMAstepgen::setEnabled(bool state)
{
	this->isEnabled = state;
}
