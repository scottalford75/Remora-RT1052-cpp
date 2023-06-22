#include "extern.h"

#include "fsl_pit.h"

void createThreads(void)
{
    baseThread = new pruThread(kPIT_Chnl_0, base_freq);

    servoThread = new pruThread(kPIT_Chnl_1 , servo_freq);
}
