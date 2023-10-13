
#include "qdc.h"

volatile bool initXBARA = true;

void muxPinsXBAR(const char* pin,xbar_output_signal_t kXBARA1_OutputEncInput)
{
  uint8_t mux_op_pin = 0;

  if(!strcmp(pin,"P4_13"))
	mux_op_pin = 1;
  else if(!strcmp(pin,"P4_14"))
	mux_op_pin = 2;
  else if(!strcmp(pin,"P3_21"))
    mux_op_pin = 3;
  else if(!strcmp(pin,"P4_00") && !strcmp(board,"EC500"))
    mux_op_pin = 4;
  else if(!strcmp(pin,"P3_23"))
    mux_op_pin = 5;
  else if(!strcmp(pin,"P4_15"))
    mux_op_pin = 6;
  else if(!strcmp(pin,"P3_16"))
  {
	mux_op_pin = 7;
	printf("P3_16 is only 5V tolerant, danger!!!!!!\r\n");
  }
  else if(!strcmp(pin,"P3_17"))
  {
	mux_op_pin = 8;
	printf("P3_17 is only 5V tolerant, danger!!!!!!\r\n");
  }
  else if(!strcmp(pin,"P3_22"))
  {
	mux_op_pin = 9;
	printf("P3_22 is only 5V tolerant, danger!!!!!!\r\n");
  }
  else if(!strcmp(pin,"P4_16") && !strcmp(board,"EC300"))
  {
	mux_op_pin = 10;
  }
  else
  {
	printf("********The %s pin cannot be multiplexed pad(pin)-/->XBAR-->Qdc********\r\n",pin);
	printf("********The instance of the Qdc module could not be carried out********\r\n");
	return;
  }

  switch(mux_op_pin)
  {
    case 1:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_13_XBAR1_IN25, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_13_XBAR1_IN25, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarIn25, kXBARA1_OutputEncInput);
		break;
    case 2:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_14_XBAR1_INOUT19, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_14_XBAR1_INOUT19, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarInout19, kXBARA1_OutputEncInput);
		break;
    case 3:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_35_XBAR1_INOUT18, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_35_XBAR1_INOUT18, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarInout18, kXBARA1_OutputEncInput);
		break;
    case 4:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_00_XBAR1_IN02, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_00_XBAR1_IN02, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarIn02, kXBARA1_OutputEncInput);
		break;
    case 5:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_37_XBAR1_IN23, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_37_XBAR1_IN23, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarIn23, kXBARA1_OutputEncInput);
		break;
    case 6:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_15_XBAR1_IN20, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_15_XBAR1_IN20, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarIn20, kXBARA1_OutputEncInput);
		break;
    case 7:
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_04_GPIO3_IO16, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_04_GPIO3_IO16, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarInout08, kXBARA1_OutputEncInput);
		break;
    case 8:
		IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_05_GPIO3_IO17, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_05_GPIO3_IO17, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarInout09, kXBARA1_OutputEncInput);
		break;
    case 9:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_36_XBAR1_IN22, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_36_XBAR1_IN22, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarIn22, kXBARA1_OutputEncInput);
		break;
    case 10:
		IOMUXC_SetPinMux(IOMUXC_GPIO_EMC_16_XBAR1_IN21, 1U);
		IOMUXC_SetPinConfig(IOMUXC_GPIO_EMC_16_XBAR1_IN21, 0x10B0U);

		XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarIn21, kXBARA1_OutputEncInput);
		break;
    default:
    	break;
  }
}

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON
************************************************************************/
void createQdc()
{
    const char* comment = module["Comment"];
    printf("%s\n",comment);

    int pv = module["PV[i]"];
    const char* pinA = module["ChA Pin"];
    const char* pinB = module["ChB Pin"];
    const char* pinI = module["Index Pin"];
    int dataBit = module["Data Bit"];
    int encNumber = module["ENC No"];
    int filt_per = module["Filter PER"];
    int filt_cnt = module["Filter CNT"];

    ENC_Type* base = nullptr;
    IRQn_Type encIndexIrqId;


    if(initXBARA == true)
    {
    	XBARA_Init(XBARA1);
    	initXBARA = false;
    }

    switch(encNumber)
    {
      case(1):
		muxPinsXBAR(pinA,kXBARA1_OutputEnc1PhaseAInput);
		muxPinsXBAR(pinB,kXBARA1_OutputEnc1PhaseBInput);
		if(pinI != nullptr)
		{
		  muxPinsXBAR(pinI,kXBARA1_OutputEnc1Index);
		}
		base = ENC1;
		encIndexIrqId = ENC1_IRQn;
      	break;
      case(2):
		muxPinsXBAR(pinA,kXBARA1_OutputEnc2PhaseAInput);
		muxPinsXBAR(pinB,kXBARA1_OutputEnc2PhaseBInput);
		if(pinI != nullptr)
		{
		  muxPinsXBAR(pinI,kXBARA1_OutputEnc2Index);
		}
		base = ENC2;
		encIndexIrqId = ENC2_IRQn;
      	break;
      case(3):
		muxPinsXBAR(pinA,kXBARA1_OutputEnc3PhaseAInput);
		muxPinsXBAR(pinB,kXBARA1_OutputEnc3PhaseBInput);
		if(pinI != nullptr)
		{
	      muxPinsXBAR(pinI,kXBARA1_OutputEnc3Index);
		}
		base = ENC3;
		encIndexIrqId = ENC3_IRQn;
      	break;
      case(4):
		muxPinsXBAR(pinA,kXBARA1_OutputEnc4PhaseAInput);
		muxPinsXBAR(pinB,kXBARA1_OutputEnc4PhaseBInput);
		if(pinI != nullptr)
		{
	      muxPinsXBAR(pinI,kXBARA1_OutputEnc4Index);
		}
		base = ENC4;
		encIndexIrqId = ENC4_IRQn;
      	break;
      default:
    	break;
    }


    ptrProcessVariable[pv]  = &txData.processVariable[pv];
    ptrInputs = &txData.inputs;

    if (pinI == nullptr)
    {
        Module* qdc = new Qdc(*ptrProcessVariable[pv],base, filt_per, filt_cnt);
        baseThread->registerModule(qdc);
    }
    else
    {

        printf("  Quadrature Encoder has index at pin %s\n", pinI);
        Module* qdc = new Qdc(*ptrProcessVariable[pv], *ptrInputs, base, encIndexIrqId, dataBit, filt_per, filt_cnt);
        //NVIC_SetPriority(encIndexIrqId , 4);
        baseThread->registerModule(qdc);
    }


}

/***********************************************************************
*                METHOD DEFINITIONS                                    *
************************************************************************/

Qdc::Qdc(volatile float &ptrEncoderCount, ENC_Type* base, int filt_per, int filt_cnt):
	ptrEncoderCount(&ptrEncoderCount),
	base(base),
	filt_per(filt_per),
	filt_cnt(filt_cnt)
{
    enc_config_t mEncConfigStruct;

    /* Initialize the ENC module. */
    ENC_GetDefaultConfig(&mEncConfigStruct);
    mEncConfigStruct.filterSamplePeriod = this->filt_per;
    mEncConfigStruct.filterCount = this->filt_cnt;
    ENC_Init(this->base, &mEncConfigStruct);
    ENC_DoSoftwareLoadInitialPositionValue(this->base); /* Update the position counter with initial value. */

    this->hasIndex = false;
    this->count = 0;								// initialise the count to 0
}

Qdc::Qdc(volatile float &ptrEncoderCount, volatile uint32_t &ptrData, ENC_Type* base,IRQn_Type irq, int bitNumber, int filt_per, int filt_cnt) :
	ptrEncoderCount(&ptrEncoderCount),
    ptrData(&ptrData),
    base(base),
	irq(irq),
    bitNumber(bitNumber),
	filt_per(filt_per),
	filt_cnt(filt_cnt)
{

    interruptPtr = new QdcInterrupt(this->irq, this);

    enc_config_t mEncConfigStruct;
    /* Initialize the ENC module. */
    ENC_GetDefaultConfig(&mEncConfigStruct);
    mEncConfigStruct.filterSamplePeriod = this->filt_per;
    mEncConfigStruct.filterCount = this->filt_cnt;
    ENC_Init(this->base, &mEncConfigStruct);
    ENC_DoSoftwareLoadInitialPositionValue(this->base); /* Update the position counter with initial value. */

    ENC_EnableInterrupts(this->base,kENC_INDEXPulseInterruptEnable);
    EnableIRQ(this->irq); /* Enable the interrupt for ENC_INDEX. */
    ENC_ClearStatusFlags(this->base, kENC_INDEXPulseFlag);

    this->hasIndex = true;
    this->indexPulse = (PRU_BASEFREQ / PRU_SERVOFREQ) * 3;          // output the index pulse for 3 servo thread periods so LinuxCNC sees it
    this->indexCount = 0;
    this->count = 0;								                // initialise the count to 0
    this->pulseCount = 0;                                           // number of base thread periods to pulse the index output
    this->mask = 1 << this->bitNumber;
    this->indexDetected = false;
}

void Qdc::update()
{
  this->count = ENC_GetPositionValue(this->base);

  if (this->hasIndex)                                     // we have an index pin
  {
      // handle index, index pulse and pulse count
      if (this->indexDetected && (this->pulseCount == 0))    // index interrupt occured: rising edge on index pulse
      {
          *(this->ptrEncoderCount) = this->count;
          this->pulseCount = this->indexPulse;
          *(this->ptrData) |= this->mask;                 // set bit in data source high
      }
      else if (this->pulseCount > 0)                      // maintain both index output and encoder count for the latch period
      {
    	  this->indexDetected = false;
          this->pulseCount--;                             // decrement the counter
      }
      else
      {
          *(this->ptrData) &= ~this->mask;                // set bit in data source low
          *(this->ptrEncoderCount) = this->count;         // update encoder count
      }
  }
  else
  {
      *(this->ptrEncoderCount) = this->count;             // update encoder count
  }
}

void Qdc::indexEvent()
{
	this->indexDetected = true;
}
