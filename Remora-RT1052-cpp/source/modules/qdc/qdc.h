#ifndef QDC_H
#define QDC_H

#include <cstdint>
#include <iostream>
#include <string>

#include "configuration.h"
#include "modules/module.h"
#include "qdcInterrupt.h"

#include "fsl_iomuxc.h"
#include "fsl_enc.h"
#include "fsl_xbara.h"
#include "fsl_xbarb.h"

#include "extern.h"

void muxPinsXBAR(const char*,xbar_output_signal_t);
void createQdc(void);

class QdcInterrupt; // forward declaration

class Qdc : public Module
{

	friend class QdcInterrupt;

	private:

		std::string ChA;			// physical pin connection
        std::string ChB;			// physical pin connection
        std::string Index;			// physical pin connection
        bool hasIndex;

        volatile float *ptrEncoderCount; 	// pointer to the data source
        volatile uint32_t *ptrData; 		// pointer to the data source

        int bitNumber;				// location in the data source
        int mask;

        ENC_Type* base;
        IRQn_Type irq;
        QdcInterrupt* 	interruptPtr;
        void indexEvent();

        uint8_t state;
        int32_t count;
        int32_t indexCount;
        int8_t  indexPulse;
        int8_t  pulseCount;
        bool    indexDetected;

	public:

        Pin* pinA;      // channel A
        Pin* pinB;      // channel B
        Pin* pinI;      // index

        Qdc(volatile float&, ENC_Type*);
        Qdc(volatile float&, volatile uint32_t&, int, ENC_Type*,IRQn_Type);

        virtual void update(void);	// Module default interface
};

#endif
