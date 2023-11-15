#include <cstdio>

#include "pruThread.h"
#include "../modules/module.h"


using namespace std;

// Thread constructor
pruThread::pruThread(GPT_Type* timer, IRQn_Type irq, uint32_t frequency) :
	timer(timer),
	irq(irq),
	frequency(frequency)
{
	printf("Creating timer ISR thread %d\n", (int)this->frequency);
	this->isISRthread = true;
}


pruThread::pruThread(DMA_Type* DMAn, uint32_t frequency) :
	DMAn(DMAn),
	frequency(frequency)
{
	printf("Creating DMA thread %d\n", (int)this->frequency);
	DMAptr = new DMA(this->DMAn, this->frequency);
	this->DMAptr->configDMA();
	this->isDMAthread = true;
}


void pruThread::startThread(void)
{
	if (isISRthread)
	{
		TimerPtr = new pruTimer(this->timer, this->irq, this->frequency, this);
	}
	else if (isDMAthread)
	{
		this->DMAptr->startDMA();
	}
}

void pruThread::stopThread(void)
{
	if (isISRthread)
	{
	    this->TimerPtr->stopTimer();
	}
	else if (isDMAthread)
	{
		this->DMAptr->stopDMA();
	}

}


void pruThread::registerModule(Module* module)
{
	this->vThread.push_back(module);
}


void pruThread::registerModulePost(Module* module)
{
	this->vThreadPost.push_back(module);
	this->hasThreadPost = true;
}


void pruThread::run(void)
{
	// iterate over the Thread pointer vector to run all instances of Module::runModule()
	for (iter = vThread.begin(); iter != vThread.end(); ++iter) (*iter)->runModule();

	// iterate over the second vector that contains module pointers to run after (post) the main vector

	if (hasThreadPost)
	{
		for (iter = vThreadPost.begin(); iter != vThreadPost.end(); ++iter) (*iter)->runModulePost();
	}
}
