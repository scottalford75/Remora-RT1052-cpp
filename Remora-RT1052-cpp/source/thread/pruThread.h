#ifndef PRUTHREAD_H
#define PRUTHREAD_H

#include "MIMXRT1052.h"
#include "timer.h"
#include "dma.h"

// Standard Template Library (STL) includes
#include <vector>

using namespace std;

class Module;
class DMA;

class pruThread
{

	private:

		pruTimer* 		    TimerPtr;
	
		GPT_Type* 	    	timer;
		DMA_Type*			DMAn;
		IRQn_Type 			irq;
		uint32_t 			frequency;

		bool isISRthread;
		bool isDMAthread;
		bool hasThreadPost;		// run updatePost() vector

		vector<Module*> vThread;		// vector containing pointers to Thread modules
		vector<Module*> vThreadPost;		// vector containing pointers to Thread modules that run after the main vector modules
		vector<Module*>::iterator iter;

	public:

		pruThread(GPT_Type*, IRQn_Type, uint32_t);
		pruThread(DMA_Type*, uint32_t);

		void registerModule(Module *module);
		void registerModulePost(Module *module);
		void startThread(void);
        void stopThread(void);
		void run(void);

		DMA*				DMAptr;
};

#endif

