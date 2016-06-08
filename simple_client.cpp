#include "ocstack.h"
#include "OCApi.h"
#include "OCPlatform.h"
#include "OCResource.h"
#include "sio_client.h"


using namespace std;

sio::socket::ptr currentSocket;
map<string, shared_ptr<OC::OCResource> > discoveredResouceMap;

void onGet(const OC::HeaderOptions& /*headerOptions*/,const OC::OCRepresentation& rep, const int eCode)
{

	if(eCode == OC_STACK_OK)
	{
		cout<<"GET request sucessfull"<<endl;
		
		cout<<"Resource URI: "<<rep.getUri()<<endl;
		
		sio::message::ptr resource_message = sio::object_message::create();
		auto &map = resource_message->get_map();
		
		map["id"] = sio::string_message::create(rep.getHost()+rep.getUri());
		
		sio::message::ptr attr_message = sio::array_message::create();	
		auto &vector = attr_message->get_vector();
	
		for(auto it = rep.begin(); it != rep.end(); ++it)
 	  	{
 	   		cout << "\tAttribute name: "<< it->attrname() << " value: ";
 	   		cout << it->getValueToString()<<endl;
 	   		sio::message::ptr message = sio::object_message::create();
 	   		auto &map = message->get_map();
 	   		map["name"] = sio::string_message::create(it->attrname());
 	   		map["value"] = sio::string_message::create(it->getValueToString());
 	   		vector.push_back(message); 
 	   		
    	}
    	
    	map["attrs"] = attr_message; 
    	currentSocket->emit("get response", resource_message);
		
	}else 
	{
		cout<<"GET request failed"<<endl;
	}
}

void onPut(const OC::HeaderOptions& /*headerOptions*/, const OC::OCRepresentation& rep, const int eCode)
{
	try
	{
		if(eCode == OC_STACK_OK)
		{
			cout<<"PUT request sucessfull"<<endl;
		
			cout<<"Resource URI: "<<rep.getUri()<<endl;
		
			for(auto it = rep.begin(); it != rep.end(); ++it)
	 	  	{
	 	   		cout << "\tAttribute name: "<< it->attrname() << " value: ";
	 	   		cout << it->getValueToString()<<endl;
			}
		
		}else 
		{
			cout<<"PUT request failed"<<endl;
		}
	}catch(exception& e)
	{
	
	}
}

void initGetRequest(shared_ptr<OC::OCResource> resource)
{
	if(resource)
	{
		cout<<"Processando o get"<<endl;
		
		OC::QueryParamsMap test;
		resource->get(test, &onGet);
	}
}

void initPutRequest(shared_ptr<OC::OCResource> resource, map<string, sio::message::ptr>& putValues)
{
	if(resource)
	{
		cout<<"Processando o put"<<endl;
		
		OC::QueryParamsMap test;
		OC::OCRepresentation rep;
		/*for(auto it = putValues.begin(); it != putValues.end(); ++it)
		{
			if(it->first != "identifier")
			{
				rep.setValue(it->first,it->second);
			}
		}*/
		
		resource->put(rep, test, &onPut);
	}
}

void foundResource(shared_ptr<OC::OCResource> resource)
{
	string resourceUri;
	string hostAddress;
	
	try{
		if(resource)
		{
			cout<<"Discovered resource"<<endl;
			
			ostringstream id;
			
			cout<<"Resource ID: "<<id.str()<<endl;
			
			resourceUri = resource->uri();
			cout<<"Resource URI: "<<resourceUri<<endl;
			
			hostAddress = resource->host();
			cout<<"Resource Address: "<<hostAddress;
			
			id <<hostAddress<<resourceUri;
			
			discoveredResouceMap[id.str()] = resource;
			
			cout << "List of resource types: " << std::endl;
      		for(auto &resourceTypes : resource->getResourceTypes())
     		{
        		cout << "\t\t" << resourceTypes << std::endl;
      		}
 
      		// Get the resource interfaces
      		cout << "List of resource interfaces: " << std::endl;
    		for(auto &resourceInterfaces : resource->getResourceInterfaces())
      		{
        		cout << "\t\t" << resourceInterfaces << std::endl;
      		}
      		
      		currentSocket->emit("discovery response", sio::string_message::create(id.str()));
      		
		}else
		{
			cout<<"Resource not found!"<<endl;
		}
	}catch(exception& e)
	{
		cout<<"Error "<<e.what()<<endl;
	}
}


void getEvent(sio::event& e)
{
	sio::message::ptr message = e.get_message();
	string identifier;
	auto &map = message->get_map();
	identifier = map["identifier"]->get_string();
	
	initGetRequest(discoveredResouceMap[identifier]);

}

void putEvent(sio::event& e)
{
	sio::message::ptr message = e.get_message();
	string identifier;
	auto &map = message->get_map();
	identifier = map["identifier"]->get_string();
	
	
	initPutRequest(discoveredResouceMap[identifier], map);
	
}

void discoveryEvent(sio::event &)
{
	if( OC::OCPlatform::findResource("",OC_RSRVD_WELL_KNOWN_URI,CT_DEFAULT ,&foundResource) != OC_STACK_OK)
	{
		cout<<"Nao achou nada ou erro ao achar"<<endl;
	}else
	{
		cout<<"No Errors"<<endl;
	}
}
int main()
{
	OC::PlatformConfig m_platform {
		OC::ServiceType::InProc, 
		OC::ModeType::Client, 
		"0.0.0.0",
        0, 
        OC::QualityOfService::HighQos
        };
	
	OC::OCPlatform::Configure(m_platform);
	
	sio::client h;
	h.connect("http://hassenco.com");
	
	currentSocket = h.socket();
	
	currentSocket->on("get", &getEvent);
	currentSocket->on("put", &putEvent);
	currentSocket->on("discovery", &discoveryEvent);

	printf("Entering infinite loop\n");
	while(true){}
	
	return 0;
}
