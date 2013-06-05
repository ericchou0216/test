/*
 * Serialization.hpp
 *
 *  Created on: May 10, 2013
 *      Author: vahid
 */
#pragma once
#include <sstream>
#include "entities/commsim/communicator/service/services.hpp"
#include "logging/Log.hpp"
#include <set>
#include <json/json.h>
namespace sim_mob
{

class JsonParser
{
public:
	//todo find a way for this hardcoding
	static sim_mob::SIM_MOB_SERVICE getServiceType(std::string type)
	{
//		Print() << "Inside getServiceType, input '" << type << "'" ;
			if(ServiceMap.find(type) == sim_mob::ServiceMap.end())
			{
				return SIMMOB_SRV_UNKNOWN;
			}
//			Print() << "   returning output '" << sim_mob::ServiceMap[type] << "'" << std::endl;
			return sim_mob::ServiceMap[type];
	}

	static bool parsePacketHeader(std::string& input, pckt_header &output, Json::Value &root){
		Json::Value packet_header;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(input, root, false);
		if(not parsedSuccess)
		{
			std::cout << "Parsing Packet Header for '" << input << "' Failed" << std::endl;
			return false;
		}
		int i = 0;
		if(root.isMember("PACKET_HEADER"))
		{
			packet_header = root["PACKET_HEADER"];
		}
		else
		{

			std::cout << "Packet header not found.Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		i += packet_header.isMember("SENDER") ? 2 : 0;
		i += packet_header.isMember("SENDER_TYPE") ? 4 : 0;
		i += packet_header.isMember("NOF_MESSAGES") ? 8 : 0;
		i += packet_header.isMember("PACKET_SIZE") ? 16 : 0;
		if(!(
				(packet_header.isMember("SENDER"))
				&&(packet_header.isMember("SENDER_TYPE"))
				&&(packet_header.isMember("NOF_MESSAGES"))
				&&(packet_header.isMember("PACKET_SIZE"))
				))
		{
			std::cout << "Packet header incomplete[" << i << "].Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		output.sender_id = packet_header["SENDER"].asString();
		output.sender_type = packet_header["SENDER_TYPE"].asString();
		output.nof_msgs = packet_header["NOF_MESSAGES"].asString();
		output.size_bytes = packet_header["PACKET_SIZE"].asString();
		return true;
	}

	static bool parseMessageHeader(std::string& input, msg_header &output){
		Json::Value root;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(input, root, false);
		if(not parsedSuccess)
		{
			std::cout << "Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		if(!(
				(root.isMember("SENDER"))
				&&(root.isMember("SENDER_TYPE"))
				&&(root.isMember("MESSAGE_TYPE"))
				))
		{
			std::cout << "Message Header incomplete. Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		output.sender_id = root["SENDER"].asString();
		output.sender_type = root["SENDER_TYPE"].asString();
		output.msg_type = root["MESSAGE_TYPE"].asString();
		return true;
	}

	static bool getPacketMessages(std::string& input,Json::Value &output)
	{
		Json::Value root;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(input, root, false);
		if(not parsedSuccess)
		{
			std::cout << "Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		if(!(
				(root.isMember("DATA"))
				&&(root["DATA"].isArray())
			))
		{
			std::cout << "A 'DATA' section with correct format was not found in the message. Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		//actual job
		output = root["DATA"];
		return true;
	}

	//used for whoami id, type and required services(optional)
	static bool get_WHOAMI(std::string& input, std::string & type, std::string & ID, std::set<sim_mob::SIM_MOB_SERVICE> &requiredServices)
	{
		Print() << "Inside get_WHOAMI, input'"<< input << "'" << std::endl;
		Json::Value root;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(input, root, false);
		if(not parsedSuccess)
		{
			std::cout << "Parsing [   " << input << "   ] Failed" << std::endl;
			return false;
		}
		if(!root.isMember("WHOAMI"))
		{
			std::cout << "No WHOAMI section.Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		Json::Value whoami = root["WHOAMI"];
		if(!(
				(whoami.isMember("ID"))
				&&(whoami.isMember("TYPE"))
				))
		{
			Print() << "WHOAMI format incomplete.Parsing '" << input << "' Failed" << std::endl;
			return false;
		}
		ID = whoami["ID"].asString();
		type =  whoami["TYPE"].asString();
//		Print() << "Inside get_WHOAMI :'" << whoami.toStyledString() << "'" << std::endl;

		if (!whoami["REQUIRED_SERVICES"].isNull()) {
			if (whoami["REQUIRED_SERVICES"].isArray()) {
				const Json::Value services = whoami["REQUIRED_SERVICES"];
//				Print() << "services :'" << services.toStyledString() << "'" << std::endl;
				for (unsigned int index = 0; index < services.size(); index++) {
					std::string type = services[index].asString();
					requiredServices.insert(getServiceType(type));
				}
			}
		}

		return true;
	}

	static bool get_WHOAMI_Services(std::string& input, std::set<sim_mob::SIM_MOB_SERVICE> & services)
	{

		Json::Value root;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(input, root, false);
		if(! parsedSuccess)
		{
			std::cout << "Parsing [" << input << "] Failed" << std::endl;
			return false;
		}
		if(!root.isMember("services"))
		{
			std::cout << "Parsing services in [" << input << "] Failed" << std::endl;
			return false;
		}
		const Json::Value array = root["services"];
		for(unsigned int index=0; index<array.size(); index++)
		{
//			getServiceType(array[index].asString());
			services.insert(getServiceType(array[index].asString()));
		}
	}

	static std::string makeWhoAreYou()
	{
		Json::Value whoAreYou;
		whoAreYou["MessageType"] = "WhoAreYou";
		Json::FastWriter writer;

		std::ostringstream out("");
		return writer.write(whoAreYou);
	}
//	just conveys the tick
	static std::string makeTimeData(unsigned int tick)
	{
		Json::Value time;
		time["MessageType"] = "TimeData";
		Json::Value breakDown;
		breakDown["tick"] = tick;
		time["TimeData"] = breakDown;
		Json::FastWriter writer;
		return writer.write(time);
	}
	static std::string makeLocationData(int x, int y)
	{

		Json::Value time;
		time["MessageType"] = "LocationData";
		Json::Value breakDown;
		breakDown["x"] = x;
		breakDown["y"] = y;
		time["LocationData"] = breakDown;
		Json::FastWriter writer;
		return writer.write(time);
	}
	//@originalMessage input
	//@extractedType output
	//@extractedData output
	//@root output
	static bool getMessageTypeAndData(std::string &originalMessage, std::string &extractedType, std::string &extractedData, Json::Value &root_)
	{
		Json::Value root;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(originalMessage, root, false);
		if(not parsedSuccess)
		{
			std::cout << "Parsing [" << originalMessage << "] Failed" << std::endl;
			return false;
		}
		extractedType = root["MessageType"].asString();
		//the rest of the message is actually named after the type name
		extractedData = root[extractedType.c_str()].asString();
		root_ = root;
		return true;
	}
};

}


/*
 * **************packet structure **************************
 * note:
 * a)packets contain sections: PACKET_HEADER , DATA
 * b)DATA section is an array of messages
 * c)each message contain message meta data elements(SENDER,SENDER_TYPE,MESSAGE_TYPE) and message specific data under a section called MESSAGE_TYPE

1-sample packets exchanged between client and server(version2, multiple messages in a single packet) :
{
    "PACKET_HEADER": {
        "SENDER"  : "cloient-or-server-id",
        "SENDER_TYPE" : "cloient-or-server-type",
        "NOF_MESSAGES" : 1,
        "PACKET_SIZE" : 123

    },
    "DATA":[
        {
            "SENDER" : "cloient-or-server-id_1",
            "SENDER_TYPE" : "cloient-or-server-type_1",
            "MESSAGE_TYPE" : "TYPE_XXX",
            "TYPE_XXX" :{
            (message data here)
            }

        }
        ,

        {
            "SENDER" : "cloient-or-server-id_2",
            "SENDER_TYPE" : "cloient-or-server-type_2",
            "MESSAGE_TYPE" : "TYPE_YYY",
            "TYPE_YYY" :{
            (message data here)
            }

        }
        ]
}

2- general message structure exchanged between clients and server:

        {
            "MessageType" : "type-name",
            "type-name" :
                {
                    "value-name" : data
                }
        }

 //       sample :

 * packet name : whoareyou
 * sending direction: server->client

{
    "PACKET_HEADER": {
        "SENDER"  : "BROKER_MAIN",
        "SENDER_TYPE" : "SIMMOB_BROKER",
        "NOF_MESSAGES" : 1,
        "PACKET_SIZE" : 123

    },
    "DATA":[
        {
            "SENDER" : "BROKER_MAIN",
            "SENDER_TYPE" : "SIMMOB_BROKER",
            "MESSAGE_TYPE" : "WHOAREYOU",
            "WHOAREYOU" :{}               <--(no data required in this case)

        }
        ]
}

 * packet name : whoami
 * sending direction: client->server
 * DESCRIPTION:
 * 1-client types can be android emulator, ns3 etc.
 * 2-apart from adhoc messages sent by client and handled by server,
 * each client will ask server to send him specific data regularly (for example: time and location at each tick)

 {
    "PACKET_HEADER": {
        "SENDER"  : "client-id-X",
        "SENDER_TYPE" : "client-type-XX",
        "NOF_MESSAGES" : 1,
        "PACKET_SIZE" : 123

    },
    "DATA":[
        {
            "SENDER" : "client-id-X",
            "SENDER_TYPE" : "ANDROID_EMULATOR",
            "MESSAGE_TYPE" : "WHOAMI",
            "WHOAMI" :{
                "SENDER" : "client-id-X",
                "SENDER_TYPE" : "ANDROID_EMULATOR",
        	    "REQUIRED_SERVICES" : [
        							    "TIME",
        							    "LOCATION"
        						      ]
            }

        }
        ]
}


before continuing, please note the possible scenarios(depending on configuration settings and available implementations) :
            simmobility                or                simmobility
               /     \                                       |
              /       \                                      |
          android      ns3                                android

 * packet name : announce
 * sending direction:
 * usage-1) client->server 		(FROM:android TO: simMobility DESCRIPTION: android client asks simmobility to deliver the message to the other android recipients)
 * usage-2) server->client 		(FROM:simMobility TO: ns3  DESCRIPTION: simmobility delegates the network transfering of messages to ns3)
{
    "MSG_TYPE" : "ANNOUNCE",
    "ANNOUNCE" : {
        "SENDER" : "client-id-X",
        "RECEIVER" :[
            "client-id-W",
            "client-id-Y",
            "client-id-Z"
            ],
        "DATA" : "opaque-data"
    }
}

 * packet name : announce_received
 * sending direction:
 * usage-1)client->server (FROM:ns3 TO: simMobility.
 * DESCRIPTION: this message is sent by ns3 to simmobility when it is received by  a ns3 node from another ns3 node,
 * simmobility has to give this message to the corresponding android client)
 *
 * usage-2)server->client : (FROM:simmobility TO:android,
 * DESCRIPTION: simmobility transfers this message(which has been sent by ns3) to the corresponding android)
{
    "MSG_TYPE" : "ANNOUNCE",
    "ANNOUNCE" : {
        "SENDER" : "client-id-X",
        "RECEIVER" :[
            "client-id-W"
            ],
        "DATA" : "opaque-data"
    }
}

 */

