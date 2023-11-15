#include "extern.h"


void createThreads(void)
{
    baseThread = new pruThread(GPT1, GPT1_IRQn, base_freq);
    NVIC_SetPriority(GPT1_IRQn, 2);

    servoThread = new pruThread(GPT2, GPT2_IRQn , servo_freq);
    NVIC_SetPriority(GPT2_IRQn , 3);

    dmaThread = new pruThread(DMA0, dma_freq);
}
