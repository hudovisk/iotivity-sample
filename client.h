#ifndef CLIENT_H
#define CLIENT_H

#include "ocstack.h"

OCStackApplicationResult 
discoveryReqCb(void * ctx, OCDoHandle handle, 
	       OCClientResponse * clientResponse);

int 
initDiscovery();

OCStackApplicationResult
getLEDReqCb(void * ctx, OCDoHandle handle,
            OCClientResponse * clientResponse);

int 
initGetLED();

int 
initPutLED(int power, int state);

#endif
