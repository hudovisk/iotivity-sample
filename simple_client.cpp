
#include "simple_client.h"

static const std::string RESOURCE_REP_SOCKET_CMD = "get response";
static const std::string RESOURCE_SOCKET_CMD = "discovery response";

static const std::string DISCOVER_SOCKET_CMD = "discovery";
static const std::string GET_SOCKET_CMD = "get";
static const std::string PUT_SOCKET_CMD = "put";
static const std::string OBSERVE_SOCKET_CMD = "observe";
static const std::string DEOBSERVE_SOCKET_CMD = "deobserve";

typedef std::map<std::string, std::shared_ptr<OC::OCResource>> ResourceMap;

static sio::socket::ptr gSocket;
static ResourceMap gDiscoveredResourcesMap;

std::string iotAttrTypeToString(OC::AttributeType type)
{
	switch(type)
	{
		case OC::AttributeType::Null:
			return "Null";
		case OC::AttributeType::Integer:
			return "Integer";	 
		case OC::AttributeType::Double: 
			return "Double";
		case OC::AttributeType::Boolean: 
			return "Boolean";
		case OC::AttributeType::String: 
			return "String";
		case OC::AttributeType::OCRepresentation:
			return "OCRepresentation";
		case OC::AttributeType::Vector:
			return "Vector";
		default:
			std::cout<<"Unsuported Iot Type!"<<std::endl;
			return "";
	}
}

void mapSocketAttrToIot(const sio::message::ptr attr, OC::OCRepresentation* rep)
{
	auto& map = attr->get_map();
	const std::string type = map["type"]->get_string();
	const std::string name = map["name"]->get_string();
	
	if (type == "Null")
	{
		std::string value = NULL;
		rep->setValue(name, value);
	}
	else if(map["type"]->get_string() == "Integer")
	{
		int value = map["value"]->get_int();
		rep->setValue(name, value);
	}
	else if(map["type"]->get_string() == "Double")
	{
		double value = map["value"]->get_double();
		rep->setValue(name, value);
	}
	else if(map["type"]->get_string() == "Boolean")
	{
		bool value = map["value"]->get_bool();
		rep->setValue(name, value);	
	}
	else if(map["type"]->get_string() == "String")
	{
		std::string value = map["value"]->get_string();
		rep->setValue(name, value.c_str());
	}
	else 
	{
		std::cout<<"Unsuported type: " << type << std::endl;
	}
}

void mapIotAttrToSocket(const OC::OCRepresentation::AttributeItem& attrItem,
	 sio::message::ptr attr)
{
	auto& map = attr->get_map();
	const std::string type = iotAttrTypeToString(attrItem.type());
	
	map["name"] = sio::string_message::create(attrItem.attrname());
	map["type"] = sio::string_message::create(type);
	
	if(map["type"]->get_string() == "Integer")
	{
		int value = attrItem.getValue<int>();
		map["value"] = sio::int_message::create(value);
	}
	else if(map["type"]->get_string() == "Double")
	{
		double value = attrItem.getValue<double>();
		map["value"] = sio::double_message::create(value);
	}
	else if(map["type"]->get_string() == "Boolean")
	{
		bool value = attrItem.getValue<bool>();
		map["value"] = sio::bool_message::create(value);	
	}
	else if(map["type"]->get_string() == "String")
	{
		std::string value = attrItem.getValue<std::string>();
		map["value"] = sio::string_message::create(value);
	}
	else 
	{
		std::cout<<"Unsuported type: " << type << std::endl;
	}
}

void sendSocketResourceRep(const OC::OCRepresentation& rep)
{
	sio::message::ptr resource_message = sio::object_message::create();
	auto &map = resource_message->get_map();

	map["id"] = sio::string_message::create(rep.getHost()+rep.getUri());

	sio::message::ptr attr_message = sio::array_message::create();	
	auto &vector = attr_message->get_vector();
	for(auto it = rep.begin(); it != rep.end(); ++it)
  	{
   		std::cout << "\tAttribute name: "<< it->attrname() << " value: ";
   		std::cout << it->getValueToString()<<std::endl;
   		
   		sio::message::ptr message = sio::object_message::create();
   		
   		mapIotAttrToSocket(*it, message);
   		
   		vector.push_back(message);
	}
	
	map["attrs"] = attr_message; 
	map["uri"] = sio::string_message::create(rep.getUri());
	map["host"] = sio::string_message::create(rep.getHost());
	map["identifier"] = sio::string_message::create(rep.getHost() + rep.getUri());
	
	gSocket->emit(RESOURCE_REP_SOCKET_CMD, resource_message);
}

/*
 * GET Request
 */
void onIotGetCB(const OC::HeaderOptions& /*headerOptions*/,
      const OC::OCRepresentation& rep,
      const int eCode)
{
	try
	{
		if(eCode == OC_STACK_OK)
		{
			std::cout<<"GET request sucessfull"<<std::endl;
			std::cout<<"Resource URI: "<<rep.getUri()<<std::endl;
		
			sendSocketResourceRep(rep);
		}
		else 
		{
			std::cout<<"GET request failed"<<std::endl;
		}
	}
	catch(std::exception& e)
	{
		std::cout<<"std::exception on get request: "<<e.what()<<std::endl;
	}
}

void socketGetEventCB(sio::event& e)
{
	sio::message::ptr message = e.get_message();
	std::string identifier;
	auto &map = message->get_map();
	identifier = map["identifier"]->get_string();

	std::shared_ptr<OC::OCResource> resource = gDiscoveredResourcesMap[identifier]; 
	if(resource)
	{
		std::cout<<"Making a get request to: "<<resource->uri()<<std::endl;
		
		OC::QueryParamsMap test;
		resource->get(test, &onIotGetCB);
	}
	else
	{
		std::cout<<"Invalid resource to make get request"<<std::endl;
	}
}


/*
 * PUT Request
 */
void onIotPutCB(const OC::HeaderOptions& /*headerOptions*/,
	 const OC::OCRepresentation& rep, const int eCode)
{
	try
	{
		if(eCode == OC_STACK_OK)
		{
			std::cout<<"PUT request sucessfull"<<std::endl;
			std::cout<<"Resource URI: "<<rep.getUri()<<std::endl;
		
			for(auto it = rep.begin(); it != rep.end(); ++it)
	 	  	{
	 	   		std::cout << "\tAttribute name: "<< it->attrname() << " value: ";
	 	   		std::cout << it->getValueToString()<<std::endl;
			}
			
			sendSocketResourceRep(rep);
		}
		else 
		{
			std::cout<<"PUT request failed"<<std::endl;
		}
	}catch(std::exception& e)
	{
		std::cout<<"std::exception on put request: "<<e.what()<<std::endl;
	}
}

void socketPutEventCB(sio::event& e)
{
	sio::message::ptr message = e.get_message();
	auto &map = message->get_map();
	
	std::string identifier = map["identifier"]->get_string();
	std::vector<sio::message::ptr> attrs = map["attrs"]->get_vector();
	
	std::shared_ptr<OC::OCResource> resource = gDiscoveredResourcesMap[identifier];
	if(resource)
	{
		std::cout<<"Making a put request to: "<<resource->uri()<<std::endl;
		
		OC::QueryParamsMap test;
		OC::OCRepresentation rep;
	
		for(int i = 0; i < attrs.size(); i++)
		{
			mapSocketAttrToIot(attrs[i], &rep);
		}
		
		resource->put(rep, test, &onIotPutCB);
	}
	else
	{
		std::cout<<"Invalid resource to make get request"<<std::endl;	
	}
}

/*
 * Observe Request
 */
void onIotObserveCB(const OC::HeaderOptions /*headerOptions*/,
	 const OC::OCRepresentation rep, const int &eCode,
	 const int &sequenceNumber )
{
	try
	{
		if(eCode == OC_STACK_OK && sequenceNumber != OC_OBSERVE_NO_OPTION)
		{
			if(sequenceNumber == OC_OBSERVE_REGISTER) 
			{
				std::cout<<"Observe Registered"<<std::endl;
			}
			else if(sequenceNumber == OC_OBSERVE_DEREGISTER)
			{
				std::cout<<"Observe Deregister"<<std::endl;
			}
			
			std::cout<<"OBSERVE RESULT"<<std::endl;
			std::cout<<"Sequence Number "<<sequenceNumber<<std::endl;
			
			sendSocketResourceRep(rep);
		}
		else
		{
			if(sequenceNumber == OC_OBSERVE_NO_OPTION)
			{
				std::cout<<"Registration or deregistration error"<<std::endl;
			}else
			{
				std::cout<<"Error"<<std::endl;
			}
		}
	}catch(std::exception e)
	{
		std::cout<<e.what()<<std::endl;
	}
}

void socketObserveEventCB(sio::event& e)
{
	std::cout<<"Observe cmd received"<<std::endl;
	sio::message::ptr message = e.get_message();
	std::string identifier;
	auto &map = message->get_map();
	identifier = map["identifier"]->get_string();
	
	std::shared_ptr<OC::OCResource> resource = gDiscoveredResourcesMap[identifier];
	if(resource) 
	{
		OC::QueryParamsMap test;
		std::cout<<resource->uri()<<" init observe"<<std::endl;
		resource->observe(OC::ObserveType::Observe, test, &onIotObserveCB);
	}
}

void socketDeobserveEventCB(sio::event& e)
{
	std::cout<<"SIO: Deobserve cmd received"<<std::endl;
	sio::message::ptr message = e.get_message();
	std::string identifier;
	auto &map = message->get_map();
	identifier = map["identifier"]->get_string();

	std::shared_ptr<OC::OCResource> resource = gDiscoveredResourcesMap[identifier];
	if(resource) 
	{
		resource->cancelObserve();
	}
}

/*
 * Discovery Request/CB
 */
void onIotDiscoveryCB(std::shared_ptr<OC::OCResource> resource)
{
	std::string resourceUri;
	std::string hostAddress;
	
	try{
		if(resource)
		{
			std::cout<<"IOT: Discovered resource"<<std::endl;
			std::cout<<"\tUnique ID: "<<resource->uniqueIdentifier()<<std::endl;
			std::cout<<"\tResource URI: "<<resource->uri()<<std::endl;
			std::cout<<"\tResource Address: "<<resource->host()<<std::endl;
		
			std::ostringstream id;
			id<<resource->uri()<<resource->host();
			std::cout<<"\tResource ID: "<<id.str()<<std::endl;
			
			gDiscoveredResourcesMap[id.str()] = resource;
			
			std::cout << "\tList of resource types: " << std::endl;
			sio::message::ptr type_message = sio::array_message::create();	
			auto &type_vector = type_message->get_vector();
      		for(auto &resourceType : resource->getResourceTypes())
     		{
        		std::cout << "\t\t" << resourceType << std::endl;
        		type_vector.push_back(sio::string_message::create(resourceType));
      		}
 
      		// Get the resource interfaces
      		std::cout << "\tList of resource interfaces: " << std::endl;
      		sio::message::ptr interface_message = sio::array_message::create();	
			auto &interface_vector = interface_message->get_vector();
    		for(auto &resourceInterface : resource->getResourceInterfaces())
      		{
        		std::cout << "\t\t" << resourceInterface << std::endl;
        		interface_vector.push_back(sio::string_message::create(resourceInterface));
      		}
      		
      		sio::message::ptr resource_message = sio::object_message::create();
			auto &map = resource_message->get_map();
			
			map["id"] = sio::string_message::create(id.str());
			map["uri"] = sio::string_message::create(resource->uri());
			map["host"] = sio::string_message::create(resource->host());
			map["isObservable"] = sio::bool_message::create(resource->isObservable());
			map["rt"] = type_message;
			map["if"] = interface_message;
      		
      		gSocket->emit(RESOURCE_SOCKET_CMD, resource_message);
		}
		else
		{
			std::cout<<"Resource not found!"<<std::endl;
		}
	}catch(std::exception& e)
	{
		std::cout<<"Error "<<e.what()<<std::endl;
	}
}

void socketDiscoveryEventCB(sio::event &)
{
	std::cout<<"Discover starts"<<std::endl;
	if( OC::OCPlatform::findResource("",OC_RSRVD_WELL_KNOWN_URI,CT_IP_USE_V4 ,&onIotDiscoveryCB) != OC_STACK_OK)
	{
		std::cout<<"Discover with errors!"<<std::endl;
	}
	else
	{
		std::cout<<"No Errors"<<std::endl;
	}
}

sio::socket::ptr initSocket()
{
	sio::client h;
	std::string server = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJfaWQiOiI1NzU5YjNhZTZiNzNjY2I5NDg0YzQwNDciLCJuYW1lIjoiSHVkbyIsImVtYWlsIjoiaHVkb0BodWRvLmNvbSIsImlhdCI6MTQ2NTQ5OTE3NiwiZXhwIjoxNDY2MzYzMTc2fQ.sv5hNpOBWgQoggkFJyhbXqQdEPtFCl9nLdM25uvlfwE";
	
	std::map<std::string, std::string> query;
	query["token"] = server;
	
	h.connect("http://52.39.6.143", query);
	return h.socket();
}

int main()
{
	OC::PlatformConfig m_platform {
		OC::ServiceType::InProc, 
		OC::ModeType::Client, 
		CT_DEFAULT,
        CT_DEFAULT, 
        OC::QualityOfService::HighQos
        };
	
	OC::OCPlatform::Configure(m_platform);
	
	gSocket = initSocket();

	gSocket->on(GET_SOCKET_CMD, (sio::socket::event_listener) &socketGetEventCB);
	gSocket->on(PUT_SOCKET_CMD, (sio::socket::event_listener) &socketPutEventCB);
	gSocket->on(DISCOVER_SOCKET_CMD, (sio::socket::event_listener) &socketDiscoveryEventCB);
	gSocket->on(OBSERVE_SOCKET_CMD, (sio::socket::event_listener) &socketObserveEventCB);
	gSocket->on(DEOBSERVE_SOCKET_CMD, (sio::socket::event_listener) &socketDeobserveEventCB);

	printf("Entering infinite loop\n");
	while(true){}
	
	return 0;
}
