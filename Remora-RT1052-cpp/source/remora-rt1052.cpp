/*
Remora firmware for Novusun RT1052 based CNC controller boards
to allow use with LinuxCNC.

Copyright (C) Scott Alford aka scotta

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; If not, see <http://www.gnu.org/licenses/>.
*/



#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

#include "ethernet.h"

#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"
#include "tftpserver.h"

#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "fsl_pit.h"
#include "flexspi_nor_flash.h"
#include "fsl_dmamux.h"
#include "fsl_edma.h"

#include "configuration.h"
#include "remora.h"

#include "crc32.h"

// libraries
#include <sys/errno.h>
#include "lib/ArduinoJson6/ArduinoJson.h"

// drivers
#include "drivers/pin/pin.h"

// interrupts
#include "interrupt/irqHandlers.h"
#include "interrupt/interrupt.h"

// threads
#include "thread/pruThread.h"
#include "thread/createThreads.h"

// modules
#include "modules/module.h"
#include "modules/blink/blink.h"
//#include "modules/debug/debug.h"
#include "modules/DMAstepgen/DMAstepgen.h"
#include "modules/encoder/encoder.h"
#include "modules/comms/RemoraComms.h"
#include "modules/pwm/spindlePWM.h"
#include "modules/stepgen/stepgen.h"
#include "modules/digitalPin/digitalPin.h"
#include "modules/nvmpg/nvmpg.h"


// state machine
enum State {
    ST_SETUP = 0,
    ST_START,
    ST_IDLE,
    ST_RUNNING,
    ST_STOP,
    ST_RESET,
    ST_WDRESET
};

uint8_t resetCnt;
uint32_t base_freq = PRU_BASEFREQ;
uint32_t servo_freq = PRU_SERVOFREQ;

// boolean
volatile bool PRUreset;
bool configError = false;
bool hasBaseThread = false;
bool hasServoThread = false;
bool hasDMAthread = false;
bool threadsRunning = false;
bool DMAstepgenRunning = false;


// DMA stepgen double buffers
AT_NONCACHEABLE_SECTION_INIT(int32_t stepgenDMAbuffer_0[DMA_BUFFER_SIZE]);		// double buffers for port DMA transfers
AT_NONCACHEABLE_SECTION_INIT(int32_t stepgenDMAbuffer_1[DMA_BUFFER_SIZE]);
vector<Module*> vDMAthread;
vector<Module*>::iterator iterDMA;
bool DMAstepgen = false;
bool stepgenDMAbuffer = false;					// indicates which double buffer to use 0 or 1
edma_handle_t g_EDMA_Handle;
volatile bool g_transferDone = false;
AT_QUICKACCESS_SECTION_DATA_ALIGN(edma_tcd_t tcdMemoryPoolPtr[3], sizeof(edma_tcd_t));

// pointers to objects with global scope
pruThread* servoThread;
pruThread* baseThread;
RemoraComms* comms;
Module* MPG;

// unions for RX, TX and MPG data
rxData_t rxBuffer;				// temporary RX buffer
volatile rxData_t rxData;
volatile txData_t txData;
volatile bool cmdReceived;
volatile bool mpgReceived;
mpgData_t mpgData;

// pointers to data
volatile rxData_t*  ptrRxData = &rxData;
volatile txData_t*  ptrTxData = &txData;
volatile int32_t* ptrTxHeader;
volatile bool*    ptrPRUreset;
volatile int32_t* ptrJointFreqCmd[JOINTS];
volatile int32_t* ptrJointFeedback[JOINTS];
volatile uint8_t* ptrJointEnable;
volatile float*   ptrSetPoint[VARIABLES];
volatile float*   ptrProcessVariable[VARIABLES];
volatile uint32_t* ptrInputs;
volatile uint32_t* ptrOutputs;
volatile uint16_t* ptrNVMPGInputs;
volatile mpgData_t* ptrMpgData = &mpgData;


// JSON config file stuff

const char defaultConfig[] = DEFAULT_CONFIG;


// 512 bytes of metadata in front of actual JSON file
typedef struct
{
  uint32_t crc32;   		// crc32 of JSON
  uint32_t length;			// length in words for CRC calculation
  uint32_t jsonLength;  	// length in of JSON config in bytes
  uint8_t padding[500];
} metadata_t;
#define METADATA_LEN    512

volatile bool newJson;
uint32_t crc32;
FILE *jsonFile;
string strJson;
DynamicJsonDocument doc(JSON_BUFF_SIZE);
JsonObject thread;
JsonObject module;


int8_t checkJson()
{
	metadata_t* meta = (metadata_t*)(XIP_BASE + JSON_UPLOAD_ADDRESS);
	uint32_t* json = (uint32_t*)(XIP_BASE + JSON_UPLOAD_ADDRESS + METADATA_LEN);

    uint32_t table[256];
    crc32::generate_table(table);
    int mod, padding;

	// Check length is reasonable
	if (meta->length > 2 * SECTOR_SIZE)
	{
		newJson = false;
		printf("JSON Config length incorrect\n");
		return -1;
	}

    // for compatibility with STM32 hardware CRC32, the config is padded to a 32 byte boundary
    mod = meta->jsonLength % 4;
    if (mod > 0)
    {
        padding = 4 - mod;
    }
    else
    {
        padding = 0;
    }
    printf("mod = %d, padding = %d\n", mod, padding);

	// Compute CRC
    crc32 = 0;
    char* ptr = (char *)(XIP_BASE + JSON_UPLOAD_ADDRESS + METADATA_LEN);
    for (int i = 0; i < meta->jsonLength + padding; i++)
    {
        crc32 = crc32::update(table, crc32, ptr, 1);
        ptr++;
    }

	printf("Length (words) = %d\n", meta->length);
	printf("JSON length (bytes) = %d\n", meta->jsonLength);
	printf("crc32 = %x\n", crc32);

	// Check CRC
	if (crc32 != meta->crc32)
	{
		newJson = false;
		printf("JSON Config file CRC incorrect\n");
		return -1;
	}

	// JSON is OK, don't check it again
	newJson = false;
	printf("JSON Config file received Ok\n");
	return 1;
}


void moveJson()
{
	uint8_t pages, sectors;
    uint32_t i = 0;
	metadata_t* meta = (metadata_t*)(XIP_BASE + JSON_UPLOAD_ADDRESS);
	uint32_t Flash_Write_Address;

	uint16_t jsonLength = meta->jsonLength;

    // how many pages are needed to be written. The first 4 bytes of the storage location will contain the length of the JSON file
    pages = (meta->jsonLength + 4) / FLASH_PAGE_SIZE;
    if ((meta->jsonLength + 4) % FLASH_PAGE_SIZE > 0)
    {
        pages++;
    }

    sectors = pages / 16; // 16 pages per sector
    if (pages % 16 > 0)
    {
    	sectors++;
    }

    printf("pages = %d, sectors = %d\n", pages, sectors);

    uint8_t data[pages * 256] = {0};

	// store the length of the file in the 0th word
    data[0] = (uint8_t)((jsonLength & 0x00FF));
    data[1] = (uint8_t)((jsonLength & 0xFF00) >> 8);

    //The buffer argument points to the data to be written, which is of size size.
    //This size must be a multiple of the "page size", which is defined as the constant FLASH_PAGE_SIZE, with a value of 256 bytes.

    for (i = 0; i < jsonLength; i++)
    {
        data[i + 4] = *((uint8_t*)(XIP_BASE + JSON_UPLOAD_ADDRESS + METADATA_LEN + i));
    }

	// erase the old JSON config file

	// init flash
	flexspi_nor_flash_init(FLEXSPI);

	// Enter quad mode
	status_t status = flexspi_nor_enable_quad_mode(FLEXSPI);
	if (status != kStatus_Success)
	{
	  return;
	}

	Flash_Write_Address = JSON_STORAGE_ADDRESS;

	for (i = 0; i < sectors; i++)
	{
		status = flexspi_nor_flash_erase_sector(FLEXSPI, Flash_Write_Address + (i * SECTOR_SIZE));
		if (status != kStatus_Success)
		{
		  PRINTF("Erase sector failure !\r\n");
		  return;
		}
	}

	for (i = 0; i < pages; i++)
	{
		status_t status = flexspi_nor_flash_page_program(FLEXSPI, Flash_Write_Address + i * FLASH_PAGE_SIZE, (uint32_t *)(data + i * FLASH_PAGE_SIZE));
		if (status != kStatus_Success)
		{
		 PRINTF("Page program failure !\r\n");
		 return;
		}
	}

	printf("Configuration file moved\n");

}


void jsonFromFlash(std::string json)
{
    int c;
    uint32_t i = 0;
    uint32_t jsonLength;


    printf("\n1. Loading JSON configuration file from Flash memory\n");

    // read word 0 to determine length to read
    jsonLength = *(uint32_t*)(XIP_BASE + JSON_STORAGE_ADDRESS);

    if (jsonLength == 0xFFFFFFFF)
    {
    	printf("Flash storage location is empty - no config file\n");
    	printf("Using basic default configuration - 3 step generators only\n");

    	jsonLength = sizeof(defaultConfig);

    	json.resize(jsonLength);

		for (i = 0; i < jsonLength; i++)
		{
			c = defaultConfig[i];
			strJson.push_back(c);
		}
    }
    else
    {
		json.resize(jsonLength);

		for (i = 0; i < jsonLength; i++)
		{
			c = *(uint8_t*)(XIP_BASE + JSON_STORAGE_ADDRESS + 4 + i);
			strJson.push_back(c);
		}
		printf("\n%s\n", json.c_str());
    }
}


void deserialiseJSON()
{
    printf("\n2. Parsing JSON configuration file\n");

    const char *json = strJson.c_str();

    // parse the json configuration file
    DeserializationError error = deserializeJson(doc, json);

    printf("Config deserialisation - ");

    switch (error.code())
    {
        case DeserializationError::Ok:
            printf("Deserialization succeeded\n");
            break;
        case DeserializationError::InvalidInput:
            printf("Invalid input!\n");
            configError = true;
            break;
        case DeserializationError::NoMemory:
            printf("Not enough memory\n");
            configError = true;
            break;
        default:
            printf("Deserialization failed\n");
            configError = true;
            break;
    }
}


void configThreads()
{
    if (configError) return;

    printf("\n3. Configuring threads\n");

    JsonArray Threads = doc["Threads"];

    // create objects from JSON data
    for (JsonArray::iterator it=Threads.begin(); it!=Threads.end(); ++it)
    {
        thread = *it;

        const char* configor = thread["Thread"];
        uint32_t    freq = thread["Frequency"];

        if (!strcmp(configor,"Base"))
        {
            base_freq = freq;
            printf("Setting BASE thread frequency to %d\n", base_freq);
        }
        else if (!strcmp(configor,"Servo"))
        {
            servo_freq = freq;
            printf("Setting SERVO thread frequency to %d\n", servo_freq);
        }
    }
}


void loadModules(void)
{
    printf("\n4. Loading modules\n");

	// Ethernet communication monitoring
	comms = new RemoraComms();
	servoThread->registerModule(comms);
	hasServoThread = true;

    if (configError) return;

    JsonArray Modules = doc["Modules"];

    // create objects from JSON data
    for (JsonArray::iterator it=Modules.begin(); it!=Modules.end(); ++it)
    {
        module = *it;

        const char* thread = module["Thread"];
        const char* type = module["Type"];

        if (!strcmp(thread,"DMA"))
        {
            printf("\nDMA thread object\n");
            hasDMAthread = true;

            if (!strcmp(type,"DMAstepgen"))
            {
            	createDMAstepgen();
            	DMAstepgen = true;
            }
        }
        else if (!strcmp(thread,"Base"))
        {
            printf("\nBase thread object\n");
            hasBaseThread = true;

            if (!strcmp(type,"Stepgen"))
            {
                createStepgen();
            }
            else if (!strcmp(type,"Encoder"))
            {
                createEncoder();
            }
         }
        else if (!strcmp(thread,"Servo"))
        {
            printf("\nServo thread object\n");
            hasServoThread = true;

        	if (!strcmp(type,"Digital Pin"))
			{
				createDigitalPin();
			}
        	else if (!strcmp(type,"Spindle PWM"))
			{
				createSpindlePWM();
			}
        	else if (!strcmp(type,"NVMPG"))
			{
				createNVMPG();
			}
        }
    }
}

/*void debugThreadHigh()
{
    //Module* debugOnB = new Debug("PE_13", 1);
    //baseThread->registerModule(debugOnB);

    Module* debugOnS = new Debug("P1_22", 1);
    servoThread->registerModule(debugOnS);
}

void debugThreadLow()
{
    //Module* debugOffB = new Debug("PE_13", 0);
    //baseThread->registerModule(debugOffB);

    Module* debugOffS = new Debug("P1_22", 0);
    servoThread->registerModule(debugOffS);
}*/


void EDMA_Callback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
	g_transferDone = true;
}


// Interrupt service for SysTick timer.
extern "C" {
	void SysTick_Handler(void)
	{
		time_isr();
	}
}



void DMAconfig(void)
{
	// Use the Periodic Interrupt Timer to trigger the DMA transfer to the GPIO DR_TOGGLE register

	edma_transfer_config_t transferConfig;
	edma_config_t userConfig;
	pit_config_t pitConfig;

	// The Periodic Interrupt Timer (PIT) module
	CLOCK_SetMux(kCLOCK_PerclkMux, 1U);
	CLOCK_SetDiv(kCLOCK_PerclkDiv, 0U);
	PIT_GetDefaultConfig(&pitConfig);
	PIT_Init(PIT, &pitConfig);

	// PIT channel 0
	PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, CLOCK_GetFreq(kCLOCK_OscClk)/(2 * DMA_FREQ));
	PIT_StartTimer(PIT, kPIT_Chnl_0);

	/* Configure DMAMUX */
	DMAMUX_Init(DMAMUX);
	DMAMUX_EnableAlwaysOn(DMAMUX, 0, true);
	DMAMUX_EnableChannel(DMAMUX, 0);

	DMAMUX_EnablePeriodTrigger(DMAMUX, 0);

	EDMA_GetDefaultConfig(&userConfig);
	EDMA_Init(DMA0, &userConfig);
	EDMA_CreateHandle(&g_EDMA_Handle, DMA0, 0);
	EDMA_SetCallback(&g_EDMA_Handle, EDMA_Callback, NULL);
	EDMA_ResetChannel(g_EDMA_Handle.base, g_EDMA_Handle.channel);

	/* prepare descriptor 0 */
	EDMA_PrepareTransfer(&transferConfig, stepgenDMAbuffer_0, sizeof(stepgenDMAbuffer_0[0]), &GPIO1->DR_TOGGLE, sizeof(GPIO1->DR_TOGGLE),
	//EDMA_PrepareTransfer(&transferConfig, stepgenDMAbuffer_0, sizeof(stepgenDMAbuffer_0[0]), &GPIO1->DR, sizeof(GPIO1->DR),
						 sizeof(stepgenDMAbuffer_0[0]),
						 sizeof(stepgenDMAbuffer_0[0]) * DMA_BUFFER_SIZE,
						 kEDMA_MemoryToPeripheral);
	EDMA_TcdSetTransferConfig(tcdMemoryPoolPtr, &transferConfig, &tcdMemoryPoolPtr[1]);
	EDMA_TcdEnableInterrupts(&tcdMemoryPoolPtr[0], kEDMA_MajorInterruptEnable);

	/* prepare descriptor 1 */
	EDMA_PrepareTransfer(&transferConfig, stepgenDMAbuffer_1, sizeof(stepgenDMAbuffer_1[0]), &GPIO1->DR_TOGGLE, sizeof(GPIO1->DR_TOGGLE),
	//EDMA_PrepareTransfer(&transferConfig, stepgenDMAbuffer_1, sizeof(stepgenDMAbuffer_1[0]), &GPIO1->DR, sizeof(GPIO1->DR),
						 sizeof(stepgenDMAbuffer_1[0]),
						 sizeof(stepgenDMAbuffer_1[0]) * DMA_BUFFER_SIZE,
						 kEDMA_MemoryToPeripheral);
	EDMA_TcdSetTransferConfig(&tcdMemoryPoolPtr[1], &transferConfig, &tcdMemoryPoolPtr[0]);
	EDMA_TcdEnableInterrupts(&tcdMemoryPoolPtr[1], kEDMA_MajorInterruptEnable);

	EDMA_InstallTCD(DMA0, 0, tcdMemoryPoolPtr);

	//EDMA_StartTransfer(&g_EDMA_Handle);
}



int main(void)
{


	enum State currentState;
	enum State prevState;

    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    currentState = ST_SETUP;
    prevState = ST_RESET;

    printf("\nRemora RT1052 starting\n\n");


    //Module* debugOn1 = new Debug("P1_17", 1);
    //Module* debugOff1 = new Debug("P1_17", 0);
    //Module* debugOn2 = new Debug("P1_31", 1);
    //Module* debugOff2 = new Debug("P1_31", 0);

    initEthernet();

    while (1)
    {
 	   switch(currentState){
     		          case ST_SETUP:
     		              // do setup tasks
     		              if (currentState != prevState)
     		              {
     		                  printf("\n## Entering SETUP state\n\n");
     		              }
     		              prevState = currentState;

     		              jsonFromFlash(strJson);
     		              deserialiseJSON();
     		              configThreads();
     		              createThreads();
     		              //debugThreadHigh();
     		              loadModules();
     		              //debugThreadLow();
     		              udpServer_init();
     		              IAP_tftpd_init(g_EDMA_Handle);

     		              currentState = ST_START;
     		              break;

     		          case ST_START:
     		              // do start tasks
     		              if (currentState != prevState)
     		              {
     		                  printf("\n## Entering START state\n");
     		              }
     		              prevState = currentState;

     		              if (!threadsRunning)
     		              {
     		                  // Start the threads
     		            	  if (hasBaseThread)
     		            	  {
         		                  printf("\nStarting the BASE thread\n");
         		                  baseThread->startThread();
     		            	  }

     		            	  if (hasServoThread)
     		            	  {
         		                  printf("\nStarting the SERVO thread\n");
         		                  servoThread->startThread();
     		            	  }

     		            	  if (hasDMAthread)
     		            	  {
         		                  DMAconfig();
     		            	  }

     		                  threadsRunning = true;
     		              }

     		              currentState = ST_IDLE;

     		              break;


     		          case ST_IDLE:
     		              // do something when idle
     		              if (currentState != prevState)
     		              {
     		                  printf("\n## Entering IDLE state\n");
     		              }
     		              prevState = currentState;

     		              //wait for data before changing to running state
     		              if (comms->getStatus())
     		              {
     		                  currentState = ST_RUNNING;
     		              }

     		              break;

     		          case ST_RUNNING:
     		              // do running tasks
     		              if (currentState != prevState)
     		              {
     		                  printf("\n## Entering RUNNING state\n");
     		              }
     		              prevState = currentState;

     		              if (DMAstepgenRunning == false)
     		              {
     		            	  // fill the buffer and kick off DMA, we should be in sync with LinuxCNC servo thread now
     		            	 for (iterDMA = vDMAthread.begin(); iterDMA != vDMAthread.end(); ++iterDMA) (*iterDMA)->runModule();
     		            	 EDMA_StartTransfer(&g_EDMA_Handle);
     		            	 DMAstepgenRunning = true;
     		            	 printf("   Starting DMA Stepgen\n");
     		              }

     		              if (comms->getStatus() == false)
     		              {
     		            	  currentState = ST_RESET;
     		              }

     		              break;

     		          case ST_STOP:
     		              // do stop tasks
     		              if (currentState != prevState)
     		              {
     		                  printf("\n## Entering STOP state\n");
     		              }
     		              prevState = currentState;


     		              currentState = ST_STOP;
     		              break;

     		          case ST_RESET:
     		              // do reset tasks
     		              if (currentState != prevState)
     		              {
     		                  printf("\n## Entering RESET state\n");
     		              }
     		              prevState = currentState;

     		              // set all of the rxData buffer to 0
     		              // rxData.rxBuffer is volatile so need to do this the long way. memset cannot be used for volatile
     		              printf("   Resetting rxBuffer\n");
     		              {
     		                  int n = sizeof(rxData.rxBuffer);
     		                  while(n-- > 0)
     		                  {
     		                      rxData.rxBuffer[n] = 0;
     		                  }
     		              }

     		              if (DMAstepgenRunning)
     		              {
     		            	 EDMA_AbortTransfer(&g_EDMA_Handle);
     		            	 DMAconfig();
     		            	 DMAstepgenRunning = false;
     		            	 printf("   Stopping DMA Stepgen\n");
     		              }


     		              currentState = ST_IDLE;
     		              break;

     		          case ST_WDRESET:
     		        	  // force a reset
     		        	  //NVIC_SystemReset();
     		              break;
     		  }

        EthernetTasks();


    	if (g_transferDone)
    	{


    		// clear the DMA buffer ready for next use
    		if (stepgenDMAbuffer)
    		{
    			memset(stepgenDMAbuffer_0, 0, sizeof(stepgenDMAbuffer_0));
    		}
    		else
    		{
    			memset(stepgenDMAbuffer_1, 0, sizeof(stepgenDMAbuffer_1));
    		}

			// switch buffers
			stepgenDMAbuffer = !stepgenDMAbuffer;

    		// prepare the next DMA buffer
    		for (iterDMA = vDMAthread.begin(); iterDMA != vDMAthread.end(); ++iterDMA) (*iterDMA)->runModule();

    		g_transferDone = false;
    	}


        if (newJson)
		{
			printf("\n\nChecking new configuration file\n");

			if (checkJson() > 0)
			{
				printf("Moving new configuration file to Flash storage and reset\n");
				moveJson();
				NVIC_SystemReset();
			}
		}
    }
}
