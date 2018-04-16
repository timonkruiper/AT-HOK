/*
 * Routing.cpp
 *
 *  Created on: Apr 13, 2018
 *      Author: timon
 */

#include "Routing.h"

#include "ThreadSafe.h"
#include "Socket/Sender.h"
#include "Message.h"
#include <iostream>

#include "Reliability.h"

Routing::Routing() {
	// TODO Auto-generated constructor stub

}

Routing::~Routing() {
	// TODO Auto-generated destructor stub
}

Message Routing::process(std::string data){
	std::vector<std::string> vec;
	split(data,"/",vec);

	std::string sourceIP = vec[0];
	std::string destinationIP = vec[1];
	std::string timeStamp = vec[2];
	std::string message = vec[3];

	if(destinationIP == IP){
		// Received a message that is for us
		//ThreadSafe(std::cout << "Data: " << data << std::endl;)
		if(timeStamp == timeStampTemp){
			// received the message before
			Message ack(sourceIP, timeStamp, "ACK");
			Sender::sendMessage(ack);
		} else {
			timeStampTemp = timeStamp;
			if(message == "ACK"){
				rel.checkTimestamp(timeStamp);
			} else {
				Message ack(sourceIP, timeStamp, "ACK");
				Sender::sendMessage(ack);
				return Message(sourceIP, destinationIP, timeStamp, message);
			}
		}
	} else if(sourceIP == IP){
		// Received own message
		//ThreadSafe(std::cout << "Received own message" << std::endl;)
	} else if (destinationIP == MULTIGROUP){
		// Group message
		if(timeStamp == timeStampTemp){
			//ThreadSafe(std::cout << "Already received the message" << std::endl;)
		} else {
			timeStampTemp = timeStamp;
			Sender::sendMessage(Message(sourceIP, destinationIP, timeStamp, message));
			return Message(sourceIP, destinationIP, timeStamp, message);
		}
	} else {
		if(timeStamp == timeStampTemp){
			//ThreadSafe(std::cout << "Received a retransmitted message" << std::endl;)
		} else {
			timeStampTemp = timeStamp;
			Sender::sendMessage(Message(sourceIP, destinationIP, timeStamp, message));
			//ThreadSafe(std::cout << "Retransmitted the message" << std::endl;)
		}
	}
	return Message("empty","empty","empty");
}

void Routing::split(const std::string& s, const char* delim, std::vector<std::string>& v) {
	auto i = 0;
	auto pos = s.find(delim);
	while (pos != std::string::npos) {
	  v.push_back(s.substr(i, pos-i));
	  i = ++pos;
	  pos = s.find(delim, pos);

	  if (pos == std::string::npos)
		 v.push_back(s.substr(i, s.length()));
	}
}
