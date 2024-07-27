#ifndef NVMPG_H
#define NVMPG_H

#include "board.h"
#include "fsl_lpuart.h"
#include "fsl_lpuart_edma.h"
#include "fsl_dmamux.h"

#include <cstdint>
//#include <sys/errno.h>

#include "configuration.h"
#include "remora.h"
#include "../module.h"
#include "extern.h"

#define NVMPG_LPUART                LPUART5
#define LPUART_CLK_FREQ           	BOARD_DebugConsoleSrcFreq()
#define LPUART_TX_DMA_CHANNEL       1U								// DMA stepgen uses channel 0
#define LPUART_RX_DMA_CHANNEL       2U								// DMA stepgen uses channel 0
#define LPUART_TX_DMA_REQUEST       kDmaRequestMuxLPUART5Tx
#define LPUART_RX_DMA_REQUEST       kDmaRequestMuxLPUART5Rx
#define LPUART_DMAMUX_BASEADDR 		DMAMUX
#define LPUART_DMA_BASEADDR    		DMA0


void createNVMPG(void);

class NVMPG : public Module
{
	private:

		volatile mpgData_t	*ptrMpgData;
		volatile uint16_t 	*ptrData;

		uint8_t txData[53] = {'\0'};
		uint8_t rxData;
		uint8_t i = 0;

		uint16_t mask;
		bool buttonState;

		bool serialReceived = false;
		bool payloadReceived = false;

	    lpuart_config_t 		lpuartConfig;
	    edma_config_t 			config;

		lpuart_edma_handle_t	g_lpuartEdmaHandle;
		edma_handle_t 			g_lpuartTxEdmaHandle;
		edma_handle_t 			g_lpuartRxEdmaHandle;
		lpuart_transfer_t 		sendXfer;

	public:

		NVMPG(volatile mpgData_t&, volatile uint16_t&);
		virtual void update(void);
		virtual void slowUpdate(void);
		virtual void configure(void);

		static void LPUART_UserCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData);
};

#endif
