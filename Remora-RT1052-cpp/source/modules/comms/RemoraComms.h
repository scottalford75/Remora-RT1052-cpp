#ifndef REMORACOMMS_H
#define REMORACOMMS_H

#include <cstdio>

#include "configuration.h"
#include "remora.h"

#include "../module.h"
#include "../../drivers/pin/pin.h"

class RemoraComms : public Module
{
  private:

	bool		data;
	bool		status;

	uint8_t		noDataCount;

	Pin*		CommsPin;

  public:

	RemoraComms(void);

	virtual void update(void);
	void dataReceived();
	bool getStatus();
};




#endif
