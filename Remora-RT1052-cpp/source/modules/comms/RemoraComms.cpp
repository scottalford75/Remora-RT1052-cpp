#include "RemoraComms.h"


RemoraComms::RemoraComms()
{
	printf("Creating an Ethernet communication monitoring module\n");

}


void RemoraComms::update()
{
	if (data)
	{
		this->noDataCount = 0;
		this->status = true;
	}
	else
	{
		this->noDataCount++;
	}

	if (this->noDataCount > DATA_ERR_MAX)
	{
		this->noDataCount = 0;
		this->status = false;
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

