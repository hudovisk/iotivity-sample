
#include "client.h"

#include "ocpayload.h"
#include "payload_logging.h"

#include <cstdlib>
#include <signal.h>
#include <pthread.h>

#include "sio_client.h"

#define TAG "my-client"

#define DEFAULT_CONTEXT 0x99

static OCDevAddr gDeviceAddr;

static int gQuitFlag = 0;

sio::socket::ptr currentSocket;

void handleSigInt(int signum)
{
    if(signum == SIGINT) gQuitFlag = 1;
}

const char *getResult(OCStackResult result)
{
    switch (result)
    {
        case OC_STACK_OK:
            return "OC_STACK_OK";
        case OC_STACK_RESOURCE_CREATED:
            return "OC_STACK_RESOURCE_CREATED";
        case OC_STACK_RESOURCE_DELETED:
            return "OC_STACK_RESOURCE_DELETED";
        case OC_STACK_INVALID_URI:
            return "OC_STACK_INVALID_URI";
        case OC_STACK_INVALID_QUERY:
            return "OC_STACK_INVALID_QUERY";
        case OC_STACK_INVALID_IP:
            return "OC_STACK_INVALID_IP";
        case OC_STACK_INVALID_PORT:
            return "OC_STACK_INVALID_PORT";
        case OC_STACK_INVALID_CALLBACK:
            return "OC_STACK_INVALID_CALLBACK";
        case OC_STACK_INVALID_METHOD:
            return "OC_STACK_INVALID_METHOD";
        case OC_STACK_NO_MEMORY:
            return "OC_STACK_NO_MEMORY";
        case OC_STACK_COMM_ERROR:
            return "OC_STACK_COMM_ERROR";
        case OC_STACK_INVALID_PARAM:
            return "OC_STACK_INVALID_PARAM";
        case OC_STACK_NOTIMPL:
            return "OC_STACK_NOTIMPL";
        case OC_STACK_NO_RESOURCE:
            return "OC_STACK_NO_RESOURCE";
        case OC_STACK_RESOURCE_ERROR:
            return "OC_STACK_RESOURCE_ERROR";
        case OC_STACK_SLOW_RESOURCE:
            return "OC_STACK_SLOW_RESOURCE";
        case OC_STACK_NO_OBSERVERS:
            return "OC_STACK_NO_OBSERVERS";
        case OC_STACK_UNAUTHORIZED_REQ:
            return "OC_STACK_UNAUTHORIZED_REQ";
#ifdef WITH_PRESENCE
        case OC_STACK_PRESENCE_STOPPED:
            return "OC_STACK_PRESENCE_STOPPED";
        case OC_STACK_PRESENCE_TIMEOUT:
            return "OC_STACK_PRESENCE_TIMEOUT";
#endif
        case OC_STACK_ERROR:
            return "OC_STACK_ERROR";
        default:
            return "UNKNOWN";
    }
}

sio::message::ptr mapToMessage(OCResourcePayload* payload)
{
	sio::message::ptr message = sio::object_message::create();
	auto &map = message->get_map();
	
	map["uri"] = sio::string_message::create(payload->uri);
	return message;
}

OCStackApplicationResult 
discoveryReqCb(void * ctx, OCDoHandle handle, 
		OCClientResponse * clientResponse)
{
    if(clientResponse)
    { 
    	OIC_LOG_V(INFO, TAG, "Discovered at %s:%d", 
		clientResponse->devAddr.addr,
		clientResponse->devAddr.port);

	gDeviceAddr = clientResponse->devAddr;

	char port[10];
	sprintf(port, "%d", clientResponse->devAddr.port);

	sio::message::list li;
	li.push(sio::string_message::create(clientResponse->devAddr.addr));
	li.push(sio::string_message::create(port));
	//currentSocket->emit("new device", li);
	sio::message::ptr message = sio::object_message::create();
	auto &map = message->get_map();
	map["adapter"] = sio::int_message::create(gDeviceAddr.adapter);
	map["flags"] = sio::int_message::create(gDeviceAddr.flags);
	map["port"] = sio::int_message::create(gDeviceAddr.port);
	map["addr"] = sio::string_message::create(gDeviceAddr.addr);
	map["interface"] = sio::int_message::create(gDeviceAddr.interface);

	OCDiscoveryPayload* discPayload = (OCDiscoveryPayload*) clientResponse->payload;
	OCResourcePayload* resourcePayload = (OCResourcePayload*) discPayload->resources;

	sio::message::ptr resource_message = sio::array_message::create();	
	auto &vector = resource_message->get_vector();
	
	while(resourcePayload != NULL)
	{
		vector.push_back(mapToMessage(resourcePayload));
		resourcePayload = resourcePayload->next;	
	}
		
	map["resources"] = resource_message;


	currentSocket->emit("new device", message);

   	OIC_LOG_V(INFO, TAG, "===========> Discover response");

	OIC_LOG_PAYLOAD(INFO, clientResponse->payload);
    }
    else 
    {
        printf("client response null\n");
    }

    return OC_STACK_KEEP_TRANSACTION;
}

int initDiscovery()
{
    OCStackResult result;
    OCCallbackData cbData;
    const char * queryUri = "/oic/res";

    cbData.cb = discoveryReqCb;
    cbData.context = (void*) DEFAULT_CONTEXT;
    cbData.cd = NULL;

    result = OCDoResource(NULL, OC_REST_DISCOVER, queryUri, 0, 0, CT_DEFAULT, 
		          OC_HIGH_QOS, &cbData, NULL, 0);
    if (OC_STACK_OK != result) 
    {
	printf("Erro OCDORESOURCE");
    }

    return result;
}

OCStackApplicationResult
getLEDReqCb(void * ctx, OCDoHandle handle, OCClientResponse * clientResponse)
{
    
    OIC_LOG_V(INFO, TAG, "Response from %s:%d =====>", 
		clientResponse->devAddr.addr,
		clientResponse->devAddr.port);
    const char* result = getResult(clientResponse->result);
    OIC_LOG_V(INFO, TAG, "Result: %s", result);
    OIC_LOG_V(INFO, TAG, "Sequence number: %d", clientResponse->sequenceNumber);
    OIC_LOG_V(INFO, TAG, "Uri: %s", clientResponse->resourceUri);

    if(!clientResponse)
    {
	printf("Response NULL\n");
	return OC_STACK_DELETE_TRANSACTION;
    }

    if(ctx == (void*) DEFAULT_CONTEXT)
    {
        printf("Recebi get \n");
    }

    OIC_LOG_PAYLOAD(INFO, clientResponse->payload);
    
    OCRepPayload* repPayload = (OCRepPayload*)clientResponse->payload;
    
    sio::message::ptr message = sio::object_message::create();
	auto &map = message->get_map();
	
	int64_t power;
	int64_t state;
	
	OCRepPayloadGetPropInt(repPayload, "power", &power);
	OCRepPayloadGetPropInt(repPayload, "state", &state);
	const char* uri = clientResponse->resourceUri;
	
	map["uri"] = sio::string_message::create(uri);
	map["power"] = sio::int_message::create(power);
	map["state"] = sio::int_message::create(state);
	
	currentSocket->emit("get response", message);

    return OC_STACK_DELETE_TRANSACTION;
}


int initGetLED()
{
    OIC_LOG(INFO, TAG, "Making Get request");
    OCStackResult res;
    OCCallbackData cbData;
    const char * queryUri = "/a/led/";

    cbData.cb = getLEDReqCb;
    cbData.context = (void*) 0x99;
    cbData.cd = NULL;

    res = OCDoResource(NULL, OC_REST_GET, queryUri, &gDeviceAddr, NULL, CT_DEFAULT,
			OC_HIGH_QOS, &cbData, NULL, 0);

    if (OC_STACK_OK != res)
    {
	printf("Erro no get\n");
    }

   return res;
}

OCPayload* putPayload(int power, int state)
{
    OCRepPayload* payload = OCRepPayloadCreate();

    if(!payload)
    {
        OIC_LOG(ERROR, TAG, "Fail to create payload");
        return NULL;
    }

    OCRepPayloadSetPropInt(payload, "power", power);
    OCRepPayloadSetPropInt(payload, "state", state);

    return (OCPayload*) payload;
}

int initPutLED(int power, int state)
{
    OIC_LOG(INFO, TAG, "Executing PUT Request");
    OCStackResult res;
    OCCallbackData cbData;
    const char * queryUri = "/a/led/";

    cbData.cb = getLEDReqCb;
    cbData.context = (void*) DEFAULT_CONTEXT;
    cbData.cd = NULL;

    res = OCDoResource(NULL, OC_REST_PUT, queryUri, &gDeviceAddr, putPayload(power, state), CT_DEFAULT,
			OC_HIGH_QOS, &cbData, NULL, 0);

    if (OC_STACK_OK != res)
    {
	printf("Erro no get\n");
    }

   return res;
}

void putEvent(sio::event &)
{
	int power;
    int state;
	printf("Recebi o put\n");
	printf("Power: ");
	scanf("%d", &power);
	printf("State: ");
    scanf("%d", &state);
    initPutLED(power, state);
}

void* PrintMenu(void* params) {
    int option = 0;
    int power;
    int state;
    while(option != 9)
    {
            printf("1-Get request\n2-Put request\n");
	    scanf("%d", &option);
	    switch(option)
	    {
		case 1:
		    initGetLED();
		    break;
		case 2:	    
		    printf("Power: ");
		    scanf("%d", &power);
		    printf("State: ");
		    scanf("%d", &state);
		    initPutLED(power, state);
		    break;
	    }
    }

    return NULL;
}

int main() {
//    setbuf(stdout, NULL);

    if (OCInit(NULL, 0, OC_CLIENT) != OC_STACK_OK)
    {
        printf("Erro!\n");
    }

    sio::client h;
    h.connect("http://hassenco.com");

    currentSocket = h.socket();
    
    currentSocket->on("put", &putEvent);

    initDiscovery();

    printf("Discovery initialised\n");

    signal(SIGINT, handleSigInt);
    
    pthread_t threadId;
    pthread_create (&threadId, NULL, PrintMenu, NULL);

    while(!gQuitFlag)
    {
        if (OCProcess() != OC_STACK_OK)
        {
            printf("Erro, OCPROCESS");
            return 0;
        }

    }

    return 0;
}

