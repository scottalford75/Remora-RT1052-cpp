#include "RemoraComms.h"


RemoraComms::RemoraComms()
{
	printf("Creating an Ethernet communication monitoring module\n");


	this->CommsPin = new Pin(LED, OUTPUT);
	this->CommsPin->set(this->status);

}


void RemoraComms::update()
{
	if (data)
	{
		this->noDataCount = 0;
		this->status = true;
		this->CommsPin->set(!this->status);
	}
	else
	{
		this->noDataCount++;
	}

	if (this->noDataCount > DATA_ERR_MAX)
	{
		this->noDataCount = 0;
		this->status = false;
		this->CommsPin->set(!this->status);
	}

	this->data = false;
}



void RemoraComms::dataReceived()
{
	this->data= true;
}


bool RemoraComms::getStatus()
{
	return this->status;
}

