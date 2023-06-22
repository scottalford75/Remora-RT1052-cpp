#ifndef PRUTHREAD_H
#define PRUTHREAD_H

#include "MIMXRT1052.h"
#include "timer.h"

// Standard Template Library (STL) includes
#include <vector>

using namespace std;

class Module;

class pruThread
{

	private:

		pruTimer* 		    TimerPtr;
	
		GPT_Type* 	    	timer;
		IRQn_Type 			irq;
		uint32_t 			frequency;

		bool hasThreadPost;		// run updatePost() vector

		vector<Module*> vThread;		// vector containing pointers to Thread modules
		vector<Module*> vThreadPost;		// vector containing pointers to Thread modules that run after the main vector modules
		vector<Module*>::iterator iter;

	public:

		pruThread(GPT_Type* timer, IRQn_Type irq, uint32_t frequency);

		void registerModule(Module *module);
		void registerModulePost(Module *module);
		void startThread(void);
        void stopThread(void);
		void run(void);
};

#endif

