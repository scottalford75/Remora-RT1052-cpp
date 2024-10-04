#ifndef QDC_H
#define QDC_H

#include <modules/qdc/portInterrupt.h>
#include <cstdint>
#include <iostream>
#include <string>
//#include <stdio.h>

#include "configuration.h"
#include "modules/module.h"
#include "fsl_iomuxc.h"
#include "fsl_enc.h"
#include "fsl_xbara.h"
#include "fsl_xbarb.h"
#include "fsl_gpio.h"

#include "extern.h"

void muxPinsXBAR(const char*,xbar_output_signal_t);
void createQdc(void);

class portInterrupt; // forward declaration

class Qdc : public Module
{

	friend class portInterrupt;

	private:

        bool hasIndex;

        volatile float *ptrEncoderCount; 	// pointer to the data source
        volatile uint32_t *ptrData; 		// pointer to the data source

        ENC_Type* encBase;
        GPIO_Type* gpioBase;
        IRQn_Type irq;
        int indexPortNumber;
        int indexPinInNumber;
        portInterrupt* 	interruptPtr;

        void indexEvent();

        int bitNumber;				// location in the data source
        int mask;
        int filt_per;
        int filt_cnt;

        uint8_t state;
        int32_t count;
        int32_t PrevCount;
        int32_t RepetitionCounter;
        int32_t indexCount;
        int8_t  indexPulse;
        int8_t  pulseCount;
        bool    indexDetected;
        bool	FrequencyMode;

	public:

        //Qdc(volatile float&, ENC_Type*, int, int);
        Qdc(volatile float&, ENC_Type*, int, int, bool);
        Qdc(volatile float&, volatile uint32_t&, ENC_Type*, GPIO_Type*, IRQn_Type, int, int, int, int, int);
        virtual void update(void);	// Module default interface
        virtual void disableInterrupt(void);

};

#endif
