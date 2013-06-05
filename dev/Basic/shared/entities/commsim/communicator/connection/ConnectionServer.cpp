/*
 * ConnectionServer.cpp
 *
 *  Created on: May 29, 2013
 *      Author: vahid
 */

#include "ConnectionServer.hpp"
#include "Session.hpp"
#include "WhoAreYouProtocol.hpp"
#include "logging/Log.hpp"
namespace sim_mob {

void ConnectionServer::handleNewClient(session_ptr sess)
{
	//using boost_shared_ptr won't let the protocol to release(i guess).
	//Therefore I used raw pointer. the protocol will delete itself(delete this;)
	WhoAreYouProtocol *registration = new WhoAreYouProtocol(sess,*this);
	registration->start();
}


void ConnectionServer::CreatSocketAndAccept() {
	// Start an accept operation for a new connection.
	std::cout << "Accepting..." << std::endl;
	sim_mob::session_ptr new_sess(new sim_mob::Session(io_service_));
	acceptor_.async_accept(new_sess->socket(),
			boost::bind(&ConnectionServer::handle_accept, this,
					boost::asio::placeholders::error, new_sess));
}

ConnectionServer::ConnectionServer(	/*std::queue<boost::tuple<unsigned int,ClientRegistrationRequest > >*/ ClientWaitList &clientRegistrationWaitingList_,
		boost::shared_ptr<boost::mutex> Broker_Client_Mutex_,
		boost::shared_ptr<boost::condition_variable> COND_VAR_CLIENT_REQUEST_,
		unsigned short port)
:
		Broker_Client_Mutex(Broker_Client_Mutex_),
		COND_VAR_CLIENT_REQUEST(COND_VAR_CLIENT_REQUEST_),
		acceptor_(io_service_,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
		clientRegistrationWaitingList(clientRegistrationWaitingList_)
{

}

void ConnectionServer::start()
{
	io_service_thread = boost::thread(&ConnectionServer::io_service_run,this);
}

void ConnectionServer::io_service_run()
{
	acceptor_.listen();
	CreatSocketAndAccept();
	io_service_.run();
}
void ConnectionServer::handle_accept(const boost::system::error_code& e, session_ptr sess) {
	if (!e) {
		std::cout << "Connection Accepted" << std::endl;
		handleNewClient(sess);
	}
	else
	{
		std::cout << "Connection Refused" << std::endl;
	}
	CreatSocketAndAccept();
}
//void ConnectionServer::RequestClientRegistration(unsigned int ID, unsigned int type, session_ptr session_)
void ConnectionServer::RequestClientRegistration(sim_mob::ClientRegistrationRequest &request)
{
	unsigned int ID;
	unsigned int type;
	session_ptr session_;
	boost::unique_lock< boost::mutex > lock(*Broker_Client_Mutex);//todo remove comment
	std::pair<std::string,ClientRegistrationRequest > p(request.client_type, request);
	clientRegistrationWaitingList.insert(p);
	ClientWaitList::iterator it = clientRegistrationWaitingList.begin();
	Print() << it->second.session_.get() << std::endl;
	COND_VAR_CLIENT_REQUEST->notify_one();
//	std::cout << " RequestClientRegistration Done, returning" << std::endl;
}

void ConnectionServer::read_handler(const boost::system::error_code& e, std::string &data, session_ptr sess) {
	if (!e) {
		std::cout << "read Successful" << std::endl;
	} else {
		std::cout << "read Failed" << std::endl;
	}

}

void ConnectionServer::general_send_handler(const boost::system::error_code& e, session_ptr sess) {
	if (!e) {
		std::cout << "write Successful" << std::endl;
	} else {
		std::cout << "write Failed:" << e.message() <<  std::endl;
	}

}

ConnectionServer::~ConnectionServer()
{
	io_service_.stop();
	io_service_thread.join();
}

} /* namespace sim_mob */
