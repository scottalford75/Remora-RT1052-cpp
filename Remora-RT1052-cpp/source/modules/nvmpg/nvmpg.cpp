#include "nvmpg.h"

#include <stdio.h>

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON
************************************************************************/

void createNVMPG()
{
    const char* comment = module["Comment"];
    printf("\n%s\n",comment);

    ptrNVMPGInputs = &txData.NVMPGinputs;
    MPG = new NVMPG(*ptrMpgData, *ptrNVMPGInputs);
    servoThread->registerModule(MPG);
}

/***********************************************************************
                METHOD DEFINITIONS
************************************************************************/

NVMPG::NVMPG(volatile mpgData_t &ptrMpgData, volatile uint16_t &ptrData) :
	ptrMpgData(&ptrMpgData),
	ptrData(&ptrData)
{
	printf("Creating NVMPG module\n");

    LPUART_GetDefaultConfig(&lpuartConfig);
    lpuartConfig.baudRate_Bps = 115200;
    lpuartConfig.enableTx     = true;
    lpuartConfig.enableRx     = true;

    LPUART_Init(NVMPG_LPUART, &lpuartConfig, LPUART_CLK_FREQ);

    /* Init DMAMUX */
    DMAMUX_Init(LPUART_DMAMUX_BASEADDR);

    /* Set channel for LPUART */
    DMAMUX_SetSource(LPUART_DMAMUX_BASEADDR, LPUART_TX_DMA_CHANNEL, LPUART_TX_DMA_REQUEST);
    DMAMUX_SetSource(LPUART_DMAMUX_BASEADDR, LPUART_RX_DMA_CHANNEL, LPUART_RX_DMA_REQUEST);
    DMAMUX_EnableChannel(LPUART_DMAMUX_BASEADDR, LPUART_TX_DMA_CHANNEL);
    DMAMUX_EnableChannel(LPUART_DMAMUX_BASEADDR, LPUART_RX_DMA_CHANNEL);

    /* Init the EDMA module */
    EDMA_GetDefaultConfig(&config);
    EDMA_Init(LPUART_DMA_BASEADDR, &config);
    EDMA_CreateHandle(&g_lpuartTxEdmaHandle, LPUART_DMA_BASEADDR, LPUART_TX_DMA_CHANNEL);
    EDMA_CreateHandle(&g_lpuartRxEdmaHandle, LPUART_DMA_BASEADDR, LPUART_RX_DMA_CHANNEL);

    /* Create LPUART DMA handle. */
    LPUART_TransferCreateHandleEDMA(NVMPG_LPUART, &g_lpuartEdmaHandle, LPUART_UserCallback, NULL, &g_lpuartTxEdmaHandle, &g_lpuartRxEdmaHandle);

	sendXfer.data     		= this->txData;
	sendXfer.dataSize 		= sizeof(txData) - 1;
}


void NVMPG::update()
{
	bool rxStatus;

	// Poll the LPUART to see if a button push byte has been received
	rxStatus = (bool)(LPUART_GetStatusFlags(NVMPG_LPUART)  & (uint32_t)kLPUART_RxFifoEmptyFlag);

	if (!rxStatus)
	{
		// data received, RxFifo is not empty
		LPUART_ReadBlocking(NVMPG_LPUART, &this->rxData, 1);
		this->serialReceived = true;
	}


	if (this->serialReceived)
	{
		// get the button number from the low nibble, subtract 2 (buttons start from #2), NVMPG start at bit 26 in the uint64_t output structure
		mask = 1 << ((rxData & 0x0f) - 2);

		// button state is from the high nibble, x0_ is button down (logical 1), x8_ is button up (logical 0)
		buttonState = (rxData & 0x80);

		if (buttonState)
		{
			*(this->ptrData) &= ~this->mask;
		}
		else
		{
			*(this->ptrData) |= this->mask;
		}

		rxData = 0;
		this->serialReceived = false;
	}

	if (this->payloadReceived)
	{
		// copy the data to txData buffer
		for (int i = 0; i < 53; i++)
		{
			this->txData[i] =  this->ptrMpgData->payload[i+4];
		}

	    LPUART_SendEDMA(NVMPG_LPUART, &g_lpuartEdmaHandle, &sendXfer);

	    this->payloadReceived = false;
	}
}


void NVMPG::slowUpdate()
{
	return;
}

void NVMPG::configure()
{
	// use standard module configure method to set payload flag
	this->payloadReceived = true;
}

void NVMPG::LPUART_UserCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData)
{

}


