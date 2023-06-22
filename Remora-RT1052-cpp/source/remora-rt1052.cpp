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


// Interrupt service for SysTick timer.
extern "C" {
	void SysTick_Handler(void)
	{
		time_isr();
	}
}


int main(void)
{
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    initEthernet();

    PRINTF("\r\n Remora RT1052 firmware for Novusun / Digital Dream CNC controller starting\r\n");

    createThreads();

    Module* blinky = new Blink("P3_00", PRU_SERVOFREQ, 1);
    servoThread->registerModule(blinky);
    servoThread->startThread();


    while (1)
    {

        EthernetTasks();
    }
}
