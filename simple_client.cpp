#include "ocstack.h"
#include "OCApi.h"
#include "OCPlatform.h"
#include "OCResource.h"
#include "sio_client.h"


using namespace std;

sio::socket::ptr currentSocket;
map<string, shared_ptr<OC::OCResource> > discoveredResouceMap;
map<OC::AttributeType, string> typeMap;

void onGet(const OC::HeaderOptions& /*headerOptions*/,const OC::OCRepresentation& rep, const int eCode)
{
	try
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
	 	   		map["type"] = sio::string_message::create(typeMap[it->type()]);
	 	   		vector.push_back(message); 
	 	   		
			}
			
			map["attrs"] = attr_message; 
			map["uri"] = sio::string_message::create(rep.getUri());
			map["host"] = sio::string_message::create(rep.getHost());
			map["identifier"] = sio::string_message::create(rep.getHost() + rep.getUri());
			currentSocket->emit("get response", resource_message);
		
		}else 
		{
			cout<<"GET request failed"<<endl;
		}
	}catch(exception& e)
	{
		cout<<"Exception on get request: "<<e.what()<<endl;
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
			initGetRequest(discoveredResouceMap[rep.getHost()+rep.getUri()]);
		}else 
		{
			cout<<"PUT request failed"<<endl;
		}
	}catch(exception& e)
	{
		cout<<"Exception on put request: "<<e.what()<<endl;
	}
}



void initPutRequest(shared_ptr<OC::OCResource> resource, vector<sio::message::ptr>& putValues)
{
	if(resource)
	{
		cout<<"Processando o put"<<endl;
		
		OC::QueryParamsMap test;
		OC::OCRepresentation rep;
		
		for(int i = 0; i<putValues.size(); i++)
		{
				auto &map = putValues[i]->get_map();
				
				if (map["type"]->get_string() == "Null")
				{
					string value = NULL;
					rep.setValue(map["name"]->get_string(), value);
				}else if(map["type"]->get_string() == "Integer")
				{
					int value = atoi(map["value"]->get_string().c_str());
					rep.setValue(map["name"]->get_string(), value);
				}else if(map["type"]->get_string() == "Double")
				{
				
				}else if(map["type"]->get_string() == "Boolean")
				{
				
				}else if(map["type"]->get_string() == "String")
				{
				
				}else if(map["type"]->get_string() == "OCRepresentation")
				{
				
				}else if(map["type"]->get_string() == "Vector")
				{
				
				}else if(map["type"]->get_string() == "Binary")
				{
				
				}	
				
			
		}
		
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
			
			resourceUri = resource->uri();
			cout<<"Resource URI: "<<resourceUri<<endl;
			
			hostAddress = resource->host();
			cout<<"Resource Address: "<<hostAddress<<endl;
		
			id <<hostAddress<<resourceUri;
			cout<<"Resource ID: "<<id.str()<<endl;
			
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
	vector<sio::message::ptr> attrs = map["attrs"]->get_vector();
	
	initPutRequest(discoveredResouceMap[identifier], attrs);
	
}

void discoveryEvent(sio::event &)
{
	if( OC::OCPlatform::findResource("",OC_RSRVD_WELL_KNOWN_URI,CT_DEFAULT ,&foundResource) != OC_STACK_OK)
	{
		cout<<"Discovered with success!"<<endl;
	}else
	{
		cout<<"No Errors"<<endl;
	}
}

void initTypeMap()
{
	typeMap[OC::AttributeType::Null] = "Null";
	typeMap[OC::AttributeType::Integer] = "Integer";
	typeMap[OC::AttributeType::Double] = "Double";
	typeMap[OC::AttributeType::Boolean] = "Boolean";
	typeMap[OC::AttributeType::String] = "String";
	typeMap[OC::AttributeType::OCRepresentation] = "OCRepresentation";
	typeMap[OC::AttributeType::Vector] = "Vector";
	typeMap[OC::AttributeType::Binary] = "Binary";
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

	initTypeMap();
		
	sio::client h;
	string server = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJfaWQiOiI1NzU5YjNhZTZiNzNjY2I5NDg0YzQwNDciLCJuYW1lIjoiSHVkbyIsImVtYWlsIjoiaHVkb0BodWRvLmNvbSIsImlhdCI6MTQ2NTQ5OTE3NiwiZXhwIjoxNDY2MzYzMTc2fQ.sv5hNpOBWgQoggkFJyhbXqQdEPtFCl9nLdM25uvlfwE";
	map<string,string> query;
	query["token"] = server;
	h.connect("http://hassenco.com", query);
	currentSocket = h.socket();
	
	currentSocket->on("get", &getEvent);
	currentSocket->on("put", &putEvent);
	currentSocket->on("discovery", &discoveryEvent);

	printf("Entering infinite loop\n");
	while(true){}
	
	return 0;
}
