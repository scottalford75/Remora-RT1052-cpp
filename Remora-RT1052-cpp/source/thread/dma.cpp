#include <stdio.h>

#include "dma.h"

// DMA constructor
DMA::DMA(DMA_Type* DMAn, uint32_t frequency):
	DMAn(DMAn),
	frequency(frequency)
{

}


void DMA::configDMA(void)
{
	// The Periodic Interrupt Timer (PIT) module
	CLOCK_SetMux(kCLOCK_PerclkMux, 1U);
	CLOCK_SetDiv(kCLOCK_PerclkDiv, 0U);
	PIT_GetDefaultConfig(&this->pitConfig);
	PIT_Init(PIT, &this->pitConfig);

	// PIT channel 0
	PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, CLOCK_GetFreq(kCLOCK_OscClk)/(2 * this->frequency));
	PIT_StartTimer(PIT, kPIT_Chnl_0);

	/* Configure DMAMUX */
	DMAMUX_Init(DMAMUX);
	DMAMUX_EnableAlwaysOn(DMAMUX, STPGEN_DMA_CHANNEL, true);
	DMAMUX_EnableChannel(DMAMUX, STPGEN_DMA_CHANNEL);

	DMAMUX_EnablePeriodTrigger(DMAMUX, STPGEN_DMA_CHANNEL);

	EDMA_GetDefaultConfig(&this->userConfig);
	EDMA_Init(this->DMAn, &this->userConfig);
	EDMA_CreateHandle(&this->EDMA_Handle, this->DMAn, STPGEN_DMA_CHANNEL);
	EDMA_SetCallback(&this->EDMA_Handle, this->EDMA_Callback, NULL);
	EDMA_ResetChannel(this->EDMA_Handle.base, this->EDMA_Handle.channel);

	/* prepare descriptor 0 */
	EDMA_PrepareTransfer((edma_transfer_config_t *)&this->transferConfig, stepgenDMAbuffer_0, sizeof(stepgenDMAbuffer_0[0]), (uint32_t*)&GPIO1->DR_TOGGLE, sizeof(GPIO1->DR_TOGGLE),
						 sizeof(stepgenDMAbuffer_0[0]),
						 sizeof(stepgenDMAbuffer_0[0]) * DMA_BUFFER_SIZE,
						 kEDMA_MemoryToPeripheral);
	EDMA_TcdSetTransferConfig(tcdMemoryPoolPtr, &this->transferConfig, &tcdMemoryPoolPtr[1]);
	EDMA_TcdEnableInterrupts(&tcdMemoryPoolPtr[0], kEDMA_MajorInterruptEnable);

	/* prepare descriptor 1 */
	EDMA_PrepareTransfer((edma_transfer_config_t *)&this->transferConfig, stepgenDMAbuffer_1, sizeof(stepgenDMAbuffer_1[0]), (uint32_t*)&GPIO1->DR_TOGGLE, sizeof(GPIO1->DR_TOGGLE),
						 sizeof(stepgenDMAbuffer_1[0]),
						 sizeof(stepgenDMAbuffer_1[0]) * DMA_BUFFER_SIZE,
						 kEDMA_MemoryToPeripheral);
	EDMA_TcdSetTransferConfig(&tcdMemoryPoolPtr[1], &this->transferConfig, &tcdMemoryPoolPtr[0]);
	EDMA_TcdEnableInterrupts(&tcdMemoryPoolPtr[1], kEDMA_MajorInterruptEnable);

	EDMA_InstallTCD(this->DMAn, 0, tcdMemoryPoolPtr);

	this->tcd_0 = &tcdMemoryPoolPtr[0];
	this->tcd_1 = &tcdMemoryPoolPtr[1];
}


void DMA::startDMA(void)
{
	 EDMA_StartTransfer(&this->EDMA_Handle);
	 printf("   Starting DMA Stepgen\n");
}


void DMA::stopDMA(void)
{
	 EDMA_AbortTransfer(&this->EDMA_Handle);
	 EDMA_Deinit(this->DMAn);
	 this->configDMA();
	 printf("   Stopping DMA Stepgen\n");
}


void DMA::updateBuffers(void)
{
	this->tcd_next = EDMA_GetNextTCDAddress(&this->EDMA_Handle);

	if (this->tcd_next == this->tcd_0)
	{
		stepgenDMAbuffer = false;
		memset(stepgenDMAbuffer_0, 0, sizeof(stepgenDMAbuffer_0));
	}
	else if (this->tcd_next == this->tcd_1)
	{
		stepgenDMAbuffer = true;
		memset(stepgenDMAbuffer_1, 0, sizeof(stepgenDMAbuffer_1));
	}
}

