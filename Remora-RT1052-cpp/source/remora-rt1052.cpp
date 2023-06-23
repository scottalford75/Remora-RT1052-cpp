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

#include "fsl_gpio.h"
#include "fsl_iomuxc.h"

#include "configuration.h"
#include "remora.h"

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
#include "modules/comms/RemoraComms.h"
#include "modules/stepgen/stepgen.h"
#include "modules/digitalPin/digitalPin.h"


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
bool threadsRunning = false;

// pointers to objects with global scope
pruThread* servoThread;
pruThread* baseThread;
RemoraComms* comms;

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

#include "fsl_cache.h"

void jsonFromFlash(std::string json)
{
    int c;
    uint32_t i = 0;
    uint32_t jsonLength;

    typedef union
    {
    	uint8_t buffer[4];
    	uint32_t length;
    } buff_t;

    buff_t bufferLength;

    printf("\n1. Loading JSON configuration file from Flash memory\n");

    // read word 0 to determine length to read
    memcpy(bufferLength.buffer, (void*)JSON_STORAGE_ADDRESS, sizeof(bufferLength));
    jsonLength = bufferLength.length;

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
			c = *(uint8_t*)(JSON_STORAGE_ADDRESS + 4 + i);
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

    if (configError) return;

    JsonArray Modules = doc["Modules"];

    // create objects from JSON data
    for (JsonArray::iterator it=Modules.begin(); it!=Modules.end(); ++it)
    {
        module = *it;

        const char* thread = module["Thread"];
        const char* type = module["Type"];

        if (!strcmp(thread,"Base"))
        {
            printf("\nBase thread object\n");

            if (!strcmp(type,"Stepgen"))
            {
                createStepgen();
            }
         }
        else if (!strcmp(thread,"Servo"))
        {
        	if (!strcmp(type,"Digital Pin"))
			{
				createDigitalPin();
			}
        	else if (!strcmp(type,"Spindle PWM"))
			{
				//createSpindlePWM();
			}
        	else if (!strcmp(type,"NVMPG"))
			{
				//createNVMPG();
			}
        }
    }
}


// Interrupt service for SysTick timer.
extern "C" {
	void SysTick_Handler(void)
	{
		time_isr();
	}
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
     		              //IAP_tftpd_init();

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
     		                  printf("\nStarting the BASE thread\n");
     		                  baseThread->startThread();

     		                  printf("\nStarting the SERVO thread\n");
     		                  servoThread->startThread();

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

     		              currentState = ST_IDLE;
     		              break;

     		          case ST_WDRESET:
     		        	  // force a reset
     		        	  //HAL_NVIC_SystemReset();
     		              break;
     		  }

        EthernetTasks();
    }
}
