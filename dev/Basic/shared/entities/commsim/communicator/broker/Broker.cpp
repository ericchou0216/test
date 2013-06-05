//external libraries
#include "Broker.hpp"
#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <json/json.h>
//core simmobility
#include "entities/AuraManager.hpp"
#include "workers/Worker.hpp"
//communication simulator
#include "Common.hpp"
#include "entities/commsim/communicator/message/derived/roadrunner/RR_Factory.hpp"//todo :temprorary
#include "entities/commsim/comm_support/JCommunicationSupport.hpp"
#include "entities/commsim/communicator/connection/ConnectionServer.hpp"
#include "entities/commsim/communicator/connection/ConnectionHandler.hpp"
#include "entities/commsim/communicator/client-registration/base/ClinetRegistrationHandler.hpp"
#include "entities/commsim/communicator/event/subscribers/base/ClientHandler.hpp"
//temporary, used for hardcoding publishers in the constructor
#include "entities/commsim/communicator/service/derived/LocationPublisher.hpp"
#include "entities/commsim/communicator/service/derived/TimePublisher.hpp"


namespace sim_mob
{
void Broker::enable() { enabled = true; }
void Broker::disable() { enabled = false; }
bool Broker::isEnabled() const { return enabled; }
void Broker::insertSendBuffer(DataElement& value)
{
	sendBuffer.add(value);
}
Broker::Broker(const MutexStrategy& mtxStrat, int id )
: Agent(mtxStrat, id)
,enabled(true) //If a Broker is created, we assume it is enabled.
{
	//Various Initializations
	Broker_Client_Mutex.reset(new boost::mutex);
	COND_VAR_CLIENT_REQUEST.reset(new boost::condition_variable);
	connection.reset(new ConnectionServer(clientRegistrationWaitingList, Broker_Client_Mutex,COND_VAR_CLIENT_REQUEST));
	 Broker_Mutex.reset(new boost::shared_mutex);
	 Broker_Mutex_Send.reset(new boost::shared_mutex);
	 Broker_Mutex_Receive.reset(new boost::shared_mutex);
	 mutex_collection.push_back(Broker_Mutex);
	 mutex_collection.push_back(Broker_Mutex_Send);
	 mutex_collection.push_back(Broker_Mutex_Receive);
	 sendBuffer.setOwnerMutex(Broker_Mutex_Send);
	 brokerCanTickForward = false;

	 //todo, for the following maps , think of something non intrusive to broker. This is merely hardcoding-vahid
	 //publishers
	 publishers.insert( std::make_pair(SIMMOB_SRV_LOCATION, boost::shared_ptr<sim_mob::Publisher>(new sim_mob::LocationPublisher()) ));
	 publishers.insert(std::make_pair(SIMMOB_SRV_TIME, boost::shared_ptr<sim_mob::Publisher> (new sim_mob::TimePublisher())));
	 //current message factory
	 boost::shared_ptr<sim_mob::MessageFactory<std::vector<msg_ptr>&, std::string&> > factory(new sim_mob::roadrunner::RR_Factory() );
	 //note that both client types refer to the same message factory belonging to roadrunner application. we will modify this to a more generic approach later-vahid
	 messageFactories.insert(std::make_pair(ANDROID_EMULATOR, factory) );
	 messageFactories.insert(std::make_pair(NS3_SIMULATOR, factory) );
}


Broker::~Broker()
{
}
/*
 * the callback function called by connection handlers to ask
 * the Broker what to do when a message arrives(this method is to be called
 * after whoareyou, registration and connectionHandler creation....)
 */
void Broker::messageReceiveCallback(boost::shared_ptr<ConnectionHandler> cnnHandler, std::string input)
{
	//extract messages from the input string(remember that a message object carries a reference to its handler also)
//	ConnectionHandler * t = cnnHandler.get();
//	unsigned int tt = t->clientType;
	boost::shared_ptr<MessageFactory<std::vector<msg_ptr>&, std::string&> > m_f = messageFactories[cnnHandler->clientType];
	std::vector<msg_ptr> messages;
	m_f->createMessage(input, messages);
//	post the messages into the message queue one by one(add their cnnHandler also)
	for(std::vector<msg_ptr>::iterator it = messages.begin(); it != messages.end(); it++)
	{
		receiveQueue.post(boost::make_tuple(cnnHandler,*it));
	}
}

boost::shared_ptr<boost::shared_mutex>  Broker::getBrokerMutex()
{
	return Broker_Mutex;
}
 boost::shared_ptr<boost::shared_mutex> Broker::getBrokerMutexSend()
 {
 	return Broker_Mutex_Send;
 }
boost::shared_ptr<boost::shared_mutex> Broker::getBrokerMutexReceive()
{
	return Broker_Mutex_Receive;
}

std::vector<boost::shared_ptr<boost::shared_mutex > > & Broker::getBrokerMutexCollection()
{
	return mutex_collection;
}


boost::shared_ptr<boost::mutex>  Broker::getBrokerClientMutex()
{
	return Broker_Client_Mutex;
}
AgentsMap & Broker::getRegisteredAgents() {
	return registeredAgents;
}
ClientWaitList & Broker::getClientWaitingList(){
	return clientRegistrationWaitingList;
}
ClientList & Broker::getClientList(){
	return clientList;
}
PublisherList & Broker:: getPublishers()
{
	return publishers;
}

void Broker::processClientRegistrationRequests()
{
//	Print() << "Processing ClientRegistrationRequests(" << clientRegistrationWaitingList.size() << std::endl;
//	boost::unique_lock<boost::mutex> lock(*Broker_Client_Mutex);
//	Print() << "Processing ClientRegistrationRequests after Mutex" << std::endl;
	boost::shared_ptr<ClientRegistrationHandler > handler;
	ClientWaitList::iterator it_erase;//helps avoid multimap iterator invalidation
	for(ClientWaitList::iterator it = clientRegistrationWaitingList.begin(), it_end(clientRegistrationWaitingList.end()); it != it_end;/*no ++ here */  )
	{
//		Print() << "Processing ClientRegistrationRequests in loop" << std::endl;
		handler.reset();
		handler = clientRegistrationFactory.getHandler((ClientTypeMap[it->first]));
		if(handler->handle(*this,it->second))
		{
			//success: handle() just added to the client to the main client list and started its connectionHandler
			//get this request out of registration list.
			it_erase =  it++;//keep the erase candidate. dont loose it :)
			clientRegistrationWaitingList.erase(it_erase) ;
			//note: if needed,remember to do the necessary work in the
			//corresponding agent w.r.t the result of handle()
			//do this through a callback to agent's reuest
		}
		else
		{
			it++; //putting it here coz multimap is not like a vector. erase doesn't return an iterator.
		}
	}
//	Print() << "Processing ClientRegistrationRequests Done" << std::endl;
}

bool  Broker::registerEntity(sim_mob::JCommunicationSupport<std::string>* value)
{
	//we won't feedback the requesting Agent until it its association with a client(or any other condition)
	//is established. That feed back will be done through agent's registrationCallBack()
	Print()<< " registering an agent " << &value->getEntity() << std::endl;
	registeredAgents.insert(std::make_pair(&value->getEntity(), value));
	value->registrationCallBack(true,mutex_collection);
	return true;
}

void  Broker::unRegisterEntity(sim_mob::JCommunicationSupport<std::string> *value)
{
	unRegisterEntity(&value->getEntity());
}

void  Broker::unRegisterEntity(const sim_mob::Agent * agent)
{
	//search agent's list looking for this agent
	registeredAgents.erase(agent); //hopefully the agent is there
	//search the internal container also
	duplicateEntityDoneChecker.erase(agent);

	//search registered clients list looking for this agent. whoever has it, dump him
	std::pair<unsigned int , boost::shared_ptr<sim_mob::ClientHandler> > client;
	ClientList::iterator it_erase;
	for(ClientList::iterator it = clientList.begin(); it != clientList.end(); /*no it++ here */)
	{
		if(it->second->agent == agent)
		{
			it_erase = it++;
			clientList.erase(it_erase);
		}
		else
		{
			 it++;
		}
	}
}

void Broker::processIncomingData(timeslice now)
{
	//just pop off the message queue and click handl ;)
	MessageElement msgTuple;
	while(receiveQueue.pop(msgTuple))
	{
		msg_ptr &msg = msgTuple.get<1>();
		msg->supplyHandler()->handle(msg);
	}
}

bool Broker::frame_init(timeslice now){
//	Print() << "Broker ConnectionServer Starting" << std::endl;
//
//	Print() << "Broker ConnectionServer Started" << std::endl;
//	Print() << "BTW here are the mutexes :" <<
//			"Broker_Mutex[" << Broker_Mutex <<
//			"] \n Broker_Mutex_Send[" << Broker_Mutex_Send <<
//			"] \n Broker_Mutex_Receive[" << Broker_Mutex_Receive <<
//			"]" << std::endl;
	return true;
};
Entity::UpdateStatus Broker::frame_tick(timeslice now){
	return Entity::UpdateStatus::Continue;
}

bool Broker::allAgentUpdatesDone()
{
	Print() << "-------------------allAgentUpdatesDone " << registeredAgents.size() << " -------------------" << std::endl;

//	boost::unique_lock< boost::shared_mutex > lock(*NS3_Communicator_Mutex);
	AgentsMap::iterator it = registeredAgents.begin(), it_end(registeredAgents.end()), it_erase;

	int i = 0;
	while(it != it_end)
	{
		Print() << "Broker::allAgentUpdatesDone::loop=> " << i << std::endl;
		sim_mob::JCommunicationSupport<std::string> *info = (it->second);//easy read
			try
			{
				if(!info->registered)
				{
					//these agents are not actually participating, just get past them
					boost::unique_lock< boost::shared_mutex > lock(*Broker_Mutex, boost::try_to_lock);
					if(!lock)
					{
						int i = 0;
					}
					duplicateEntityDoneChecker.insert(it->first);
					Print() << "Agent " << it->first << " gets a pass" << std::endl;
					it++;
					continue;
				}
				else
				if (info->isAgentUpdateDone())
				{
					boost::unique_lock< boost::shared_mutex > lock(*Broker_Mutex);
					Print() << "Agent " << it->first << " gets a pass##" << std::endl;
					duplicateEntityDoneChecker.insert(it->first);
				}
				else
				{
					Print() << "Agent is registered but its update not done : " <<  it->first << std::endl;
				}
			}
			catch(std::exception& e)
			{
				Print() << "Exception Occured " << e.what() << std::endl;
				if(deadEntityCheck(info))
				{
//					Print()<< "Unregistering Agent " << &info->getEntity() << std::endl;
					it++;
					unRegisterEntity(info);
					continue;
				}
				else
					throw std::runtime_error("Unknown Error checking entity");
			}
			it++;
	}
	bool res = (duplicateEntityDoneChecker.size() >= registeredAgents.size());
	Print() << "\nallAgentUpdatesDone=> " << registeredAgents.size() << " " << duplicateEntityDoneChecker.size() << std::endl;
	Print() << "---------------------------------------------------------------" << std::endl;
	return res;
}

//todo: again this function is also intrusive to Broker. find a way to move the following switch cases outside the broker-vahid
//todo suggestion: for publishment, don't iterate through the list of clients, rather, iterate the publishers list, access their subscriber list and say publish and publish for their subscribers(keep the clientlist for MHing only)
void Broker::processPublishers(timeslice now) {
	std::pair<SIM_MOB_SERVICE, boost::shared_ptr<sim_mob::EventPublisher> > publisher_pair;
	BOOST_FOREACH(publisher_pair, publishers)
	{
		//easy reading
		SIM_MOB_SERVICE service = publisher_pair.first;
		sim_mob::EventPublisher & publisher = *publisher_pair.second;

		switch (service) {
		case sim_mob::SIMMOB_SRV_TIME: {
			publisher.Publish(COMMEID_TIME, TimeEventArgs(now));
			break;
		}
		case sim_mob::SIMMOB_SRV_LOCATION: {
			//you have to provide context id (agent ptr) for each client
			//iterate through each registered client
			std::pair<unsigned int, boost::shared_ptr<sim_mob::ClientHandler> > client;

			BOOST_FOREACH(client, clientList)
			{
				if (
						client.second->requiredServices.find(SIMMOB_SRV_LOCATION)
						==
						client.second->requiredServices.end()
								) {
					//you didn't signe up for this service
					continue;
				}
				//get the agent id, it is used as context id
				publisher.Publish(COMMEID_LOCATION,(void*) client.second->agent,LocationEventArgs(client.second->agent));
			}
			break;
		}
		default:
			break;
		}
//	std::pair<unsigned int , boost::shared_ptr<sim_mob::ClientHandler> > client;
//	//iterate through each registered client
//	BOOST_FOREACH(client, clientList) {
//		//get the services client is subscribed to
//		std::set<SIM_MOB_SERVICE> & requiredServices =	client.second.requiredServices;
//		SIM_MOB_SERVICE service;
//		//iterate through the client's set of services
//		BOOST_FOREACH(service, requiredServices) {
//			//a small check
//			if (publishers.find(service) == publishers.end()) {
//				continue;
//			}
//			//find a publisher
//			boost::shared_ptr<sim_mob::Publisher> publisher = publishers[service];
//			switch (publisher->myService) {
//			case sim_mob::SIMMOB_SRV_TIME: {
//				publisher->Publish(COMMEID_TIME, TimeEventArgs(timeslice));
//				break;
//			}
//			case sim_mob::SIMMOB_SRV_LOCATION: {
//				publisher->Publish(COMMEID_LOCATION,	LocationEventArgs(client.second.agent));
//				break;
//			}
//			}
	}
}
void Broker::processOutgoingData(timeslice now)
{
//	now send what you have to send:
		DataElement dataElement ;
		while(sendBuffer.pop(dataElement))
		{
			boost::shared_ptr<ConnectionHandler> cnn = dataElement.get<0>();
			std::string message =  dataElement.get<1>();
			cnn->send(message);
		}
}

//checks to see if the subscribed entity(agent) is alive
bool Broker::deadEntityCheck(sim_mob::JCommunicationSupport<std::string> * info) {
//	info.cnt_1++;
//	if (info.cnt_1 < 1000)
//		return false;
	if(!info)
	{
		throw std::runtime_error("Invalid JCommunicationSupport\n");
	}
	try {

		if (!(info->getEntity().currWorker)) {
			Print() << "currWorker dead" << std::endl;
			return true;
		}

		//one more check to see if the entity is deleted
		const std::vector<sim_mob::Entity*> & managedEntities_ =
				info->getEntity().currWorker->getEntities();
		std::vector<sim_mob::Entity*>::const_iterator it =
				managedEntities_.begin();
		if(!managedEntities_.size())
		{
			return true;
		}
		for (std::vector<sim_mob::Entity*>::const_iterator it =
				managedEntities_.begin(); it != managedEntities_.end(); it++) {
			//agent is still being managed, so it is not dead
			if (*it == &(info->getEntity()))
				return false;
		}
	} catch (std::exception& e) {
		return true;
	}

	return true;
}

//iterate the entire agent registration list looking for
//those who are not done with their update and check if they are dead.
//you better hope they are dead otherwise you have to hold the simulation
//tick waiting for them to finish
void Broker::refineSubscriptionList() {
	AgentsMap::iterator it, it_end(registeredAgents.end());
	for(it = registeredAgents.begin(); it != it_end; it++)
	{
		const sim_mob::Agent * target = (*it).first;
		//you or your worker are probably dead already. you just don't know it
		if (!target->currWorker)
			{
				unRegisterEntity(target);
				continue;
			}
		const std::vector<sim_mob::Entity*> & managedEntities_ = (target->currWorker)->getEntities();
		std::vector<sim_mob::Entity*>::const_iterator  it_entity = std::find(managedEntities_.begin(), managedEntities_.end(), target);
		if(it_entity == managedEntities_.end())
		{
			unRegisterEntity(target);
			continue;
		}
		else
		{
//			Print() << std::dec << "_Agent [" << target << ":" << *it_entity << "] is still among " << (int)((target->currWorker)->getEntities().size()) << " entities of worker[" << target->currWorker << "]" << std::endl;
		}
	}
}
//sim_mob::Broker sim_mob::Broker::instance(MtxStrat_Locked, 0);

//todo:  put a better condition here. this is just a placeholder
bool Broker::subscriptionsQualify() const
{
	return registeredAgents.size() >= MIN_AGENTS;
}
//todo:  put a better condition here. this is just a placeholder
bool Broker::clientsQualify() const
{
	return clientList.size() >= MIN_CLIENTS;
}
bool Broker::brokerCanProceed() const
{
	return (subscriptionsQualify() && clientsQualify());
}
bool Broker::waitForClients()
{

	boost::unique_lock<boost::mutex> lock(*Broker_Client_Mutex);
	processClientRegistrationRequests();
	/**if:
	 * 1- number of subscribers is too low
	 * 2-there is no client(emulator) waiting in the queue
	 * 3-this update function never started to process any data so far
	 * then:
	 *  wait for enough number of clients and agents to join
	 */
	while(!(/*brokerCanProceed() && */brokerCanTickForward)) {
		Print()<< "Broker Blocking" << std::endl;
		COND_VAR_CLIENT_REQUEST->wait(lock);
//		Print()<< "Broker notified, processing Client Registration" << std::endl;
		processClientRegistrationRequests();
		if(brokerCanProceed())
		{
			//now that you are qualified(have enough number of subscribers), then
			//you are considered to authorize ticking through rather than BLOCKING.
			brokerCanTickForward = true;
		}
	}
	Print()<< "Broker NOT Blocking" << std::endl;

	//broker started before but suddenly is no more qualified to run
	if(brokerCanTickForward && (!subscriptionsQualify())) {
		//don't block, just cooperate & don't do anything until this simulation ends
		//TODO: This might be why our client eventually gives up.
		return false;
	}

	//Success! Continue.
	return true;
}

void Broker::waitForUpdates()
{
	int i = 0;
	while(!allAgentUpdatesDone()) {
		Print() << "allAgentUpdatesDone=> " << i << std::endl;
		refineSubscriptionList();
	}
}

Entity::UpdateStatus Broker::update(timeslice now)
{
	Print() << "Broker tick:"<< now.frame() << "\n";
	//step-1 : open the door to ouside world
	if(now.frame() == 0) {
		connection->start();
	}
	Print() << "Broker tick:"<< now.frame() << " - after connection start" << std::endl;

	//Step-2: Ensure that we have enough clients to process
	//(in terms of client type (like ns3, android emulator, etc) and quantity(like enough number of android clients) ).
	//Block the simulation here(if you have to)
	if (!waitForClients()) {
		return UpdateStatus(UpdateStatus::RS_CONTINUE);
	}
	Print() << "Broker tick:"<< now.frame() << " - after waitForClients" << std::endl;
	//step-3: Process what has been received in your receive container(message queue perhaps)
	processIncomingData(now);
	Print() << "Broker tick:"<< now.frame() << " - after processIncomingData" << std::endl;
	//step-4: if need be, wait for all agents(or others)
	//to complete their tick so that you are the last one ticking)
//	if(now.frame() == 105)
//	{
//		int i = 0;
//	}
	waitForUpdates();
	Print() << "Broker tick:"<< now.frame() << " - after waitForUpdates" << std::endl;
	//step-5: signal the publishers to publish their data
	processPublishers(now);
	Print() << "Broker tick:"<< now.frame() << " - after processPublishers" << std::endl;

	//step-6: Now send what has been prepared by different sources to their corresponding destications(clients)
//	processOutgoingData(now);
	Print() << "Broker tick:"<< now.frame() << " - after processOutgoingData" << std::endl;

	//step-7: final steps that should be taken before leaving the tick
//	//this method may have already been called through waitForClients()
//	//we just see if anything is left off
//	processClientRegistrationRequests();
	//prepare for next tick.
	duplicateEntityDoneChecker.clear();
//	return proudly
	return UpdateStatus(UpdateStatus::RS_CONTINUE);

}

//void Broker::unicast(const sim_mob::Agent * agent, std::string data)
//{
//	//find the connection handler
//	agentIterator it = agentSubscribers_.find(agent);
//	if(it == agentSubscribers_.end()) {
//		Print() << "unicast Failed. Agent not Found" << std::endl;
//	}
////	ConnectionHandler handler = it->handler;
//	it->handler->send(data);
//}

}//namespace



























//data is : {"messageType":"ANNOUNCE", "ANNOUNCE" : {"Sender":"clientIdxxx", "x":"346378" , "y":"734689237", "OfferingTokens":["A", "B", "C"]}}
//no need to parse the message much:
////just update its x,y location, then forward it to nearby agents
//bool Broker::handleANNOUNCE(std::string data)
//{
//	Json::Value root;
//	Json::Reader reader;
//	bool parsedSuccess = reader.parse(data, root, false);
//	if(not parsedSuccess)
//	{
//		Print() << "Parsing [" << data << "] Failed" << std::endl;
//		return false;
//	}
//	Json::Value body = root["ANNOUNCE"];
//	unsigned int clientID = body["Sender"].asUInt();
//	//get the mapping agent
//	clientIterator it = clientSubscribers_.find(clientID);
//	const Agent * agent = it->agent;
//	//get agent's fresh position (to be used in the generated message too)
//	int x,y;
//	x = agent->xPos.get();
//	y = agent->yPos.get();
//	//update location
//	body["x"] = x;
//	body["y"] = y;
//	root["ANNOUNCE"] = body;//save back
//	//now create a fresh json string
//	Json::FastWriter writer;
//	std::string output = writer.write(root);
//	//get nearby agents
//	sim_mob::Point2D lower(x-3500, y-3500);
//	sim_mob::Point2D upper(x+3500, y+3500);
//	//get nearby agents
//	std::vector<const Agent*> nearby_agents = AuraManager::instance().agentsInRect(lower,upper);
//	for(std::vector<const Agent*>::iterator it = nearby_agents.begin(); it != nearby_agents.end(); it++)
//	{
//		sendBufferMap[*it].add(BufferContainer::makeDataElement(output,clientID,(unsigned long)(*it)));
//	}
//	return true;
//}
//
////data is : {"messageType":"KEY_REQUEST", "KEY_REQUEST" : {"Sender":"clientIdxxx", "Receiver" : "clientIdyyy", "RequestingTokens":["A", "B", "C"]}}
////just extract the receiver and forward the string to it without modifications
//bool sim_mob::Broker::handleKEY_REQUEST(std::string data)
//{
//	//parse
//	Json::Value root;
//	Json::Reader reader;
//	bool parsedSuccess = reader.parse(data, root, false);
//	if(not parsedSuccess)
//	{
//		Print() << "Parsing [" << data << "] Failed" << std::endl;
//		return false;
//	}
//	//get the destination
//	Json::Value body = root["KEY_REQUEST"];
//	unsigned int receivingClientID = body["Receiver"].asUInt();
//	unsigned int sendingClientID = body["Sender"].asUInt();
//	clientIterator it = clientSubscribers_.find(receivingClientID);
//	if(it == clientSubscribers_.end())
//	{
//		return false;
//	}
//	const sim_mob::Agent* receiver = it->agent;
//	sendBufferMap[receiver].add(BufferContainer::makeDataElement(data,sendingClientID,receivingClientID));
//	return true;
//}
//
////data is : {"messageType":"KEY_SEND", "KEY_SEND" : {"Sender":"clientIdxxx", "Receiver" : "clientIdyyy", "SendingTokens":["A", "B", "C"]}}
////just extract the receiver and forward the string to it without modifications
//bool sim_mob::Broker::handleKEY_SEND(std::string data)
//{
//	//parse
//	Json::Value root;
//	Json::Reader reader;
//	bool parsedSuccess = reader.parse(data, root, false);
//	if(not parsedSuccess)
//	{
//		Print() << "Parsing [" << data << "] Failed" << std::endl;
//		return false;
//	}
//	//get the destination
//	Json::Value body = root["KEY_SEND"];
//	unsigned int receivingClientID = body["Receiver"].asUInt();
//	unsigned int sendingClientID = body["Sender"].asUInt();
//	clientIterator it = clientSubscribers_.find(receivingClientID);
//	if(it == clientSubscribers_.end())
//	{
//		return false;
//	}
//	const sim_mob::Agent* receiver = it->agent;
//	sendBufferMap[receiver].add(BufferContainer::makeDataElement(data,sendingClientID,receivingClientID));
//	return true;
//}
