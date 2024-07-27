#ifndef DMA_H
#define DMA_H

#include "MIMXRT1052.h"
#include <stdint.h>

#include "fsl_pit.h"
#include "fsl_dmamux.h"
#include "fsl_edma.h"

#include "extern.h"

#define STPGEN_DMA_CHANNEL       0U	// NVMGP uses channel 1 and 2


class DMA
{

	private:

		DMA_Type* 	    		DMAn;
		uint32_t 				frequency;

		edma_transfer_config_t 	transferConfig;
		edma_config_t 			userConfig;
		pit_config_t 			pitConfig;

		int32_t 				tcd_0, tcd_1, tcd_next;

		static void EDMA_Callback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
		{
			DMAtransferDone = true;
		}

	public:

		DMA(DMA_Type*, uint32_t);
		void configDMA(void);
		void startDMA(void);
        void stopDMA(void);
        void updateBuffers(void);

		static bool				DMAtransferDone;
		edma_handle_t 			EDMA_Handle;
};

#endif
