#ifndef EXTERN_H
#define EXTERN_H

#include "configuration.h"
#include "remora.h"

#include "fsl_edma.h"

#include "../source/lib/ArduinoJson6/ArduinoJson.h"
#include "../source/thread/pruThread.h"
#include "../source/modules/qdc/qdc.h"

#include "modules/comms/RemoraComms.h"

extern uint32_t base_freq;
extern uint32_t servo_freq;
extern uint32_t dma_freq;

extern const char* board;
extern JsonObject module;

extern bool hasBaseThread;
extern bool hasServoThread;
extern bool hasDMAthread;
extern bool hasQDC;
extern bool DMAthreadRunning;

extern Module* qdc[MAX_INST_QDC_MOD];

// DMA stepgen double buffers
extern int32_t stepgenDMAbuffer_0[DMA_BUFFER_SIZE];
extern int32_t stepgenDMAbuffer_1[DMA_BUFFER_SIZE];
extern bool stepgenDMAbuffer;

extern edma_tcd_t tcdMemoryPoolPtr[3];

// pointers to objects with global scope
extern pruThread* baseThread;
extern pruThread* servoThread;
extern pruThread* dmaThread;
extern RemoraComms* comms;
extern Module* MPG;

// unions for RX and TX data
extern rxData_t rxBuffer;
extern volatile rxData_t rxData;
extern volatile txData_t txData;
extern mpgData_t mpgData;

// pointers to data
extern volatile rxData_t*  ptrRxData;
extern volatile txData_t*  ptrTxData;
extern volatile int32_t*   ptrTxHeader;  
extern volatile int32_t*   ptrJointFreqCmd[JOINTS];
extern volatile int32_t*   ptrJointFeedback[JOINTS];
extern volatile uint8_t*   ptrJointEnable;
extern volatile float*     ptrSetPoint[VARIABLES];
extern volatile float*     ptrProcessVariable[VARIABLES];
extern volatile uint32_t*  ptrInputs;
extern volatile uint32_t*  ptrOutputs;
extern volatile uint16_t*  ptrNVMPGInputs;
extern volatile mpgData_t* ptrMpgData;


#endif
