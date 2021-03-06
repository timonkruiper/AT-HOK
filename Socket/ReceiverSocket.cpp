/*
 * ReceiverSocket.cpp
 *
 *  Created on: Apr 12, 2018
 *      Author: timon
 */

#include "ReceiverSocket.h"

#include "BlockingQueue.h"
#include "ip_config.h"

#include "../ThreadSafe.h"

#include <cstring>


ReceiverSocket::ReceiverSocket(std::string ip, int port, std::string group, BlockingQueue<std::string>* q) {
	// TODO Auto-generated constructor stub
	blockQ = q;
	rsock = get_receive_socket(ip, port, group);
}

ReceiverSocket::~ReceiverSocket() {
	// TODO Auto-generated destructor stub

}

void ReceiverSocket::receive(){
// prepare a structure to put peer data into
	struct sockaddr_in peer_address;
	socklen_t peer_address_len;
	peer_address_len = sizeof(struct sockaddr_storage);

	// allocate memory to put the received data into
	char * data = new char[1500];
	int len;
	len = 0;

	while (1) {
		// Receive packet and put its contents in data, recvfrom will block until a packet for this socket has been received
		len = recvfrom(rsock, data, 1500, 0, (struct sockaddr *) &peer_address, &peer_address_len);
		if(len == -1){
			return;
		}
		if(len > 0){
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(peer_address.sin_addr), str, INET_ADDRSTRLEN);
			std::string receive_ip(str);
			//std::cout << "Received IP: " << receive_ip << std::endl;
			if(receive_ip != IP){
				// not received own packet
				char * message = new char[len+1];
				message[len] = '\0'; //set string termination char at end
				memcpy(message, data, len);
				blockQ->push( std::string(message) );
				delete message;
//				printf("Packet of size %d received!\nData: %s\n\n", len, message);
			}

		}
	}
}

void ReceiverSocket::closeSocket(){
#ifdef _WIN32
		WSACleanup();
#elif __linux__
		close(rsock);
#endif
}

int rsock;

/**
 * Opens the socket, attach it to an interface and join the multicast group
 */
#ifdef _WIN32
SOCKET ReceiverSocket::get_receive_socket(std::string ip, uint16_t port, std::string group) {
#elif __linux__
int ReceiverSocket::get_receive_socket(std::string ip, uint16_t port, std::string group ) {
#endif
	/**
	 * Create a new datagram socket
	 */
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// make socket
	SOCKET retsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (retsock == INVALID_SOCKET) {
		ThreadSafe(std::cout << "error opening socket!" << std::endl;)
		WSACleanup();
		exit(19);
	}
#elif __linux__
	int retsock;
	if ((retsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { //Get a IPv4 (AF_INET) socket for UDP packets (SOCK_DGRAM)
		perror("Failed to create socket!");
		exit(9);
	}
#endif
	/**
	 * Setting the reuse var to 1 so multiple programs can read from this port
	 * This is only useful if you want multiple clients to be able to listen on this port, for example when testing your application locally
	 * It only really makes sense to use it this way with multicast traffic
	 */
	int reuse = 0;
	if(setsockopt(retsock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		perror("Failed to set SO_REUSEADDR!");
#ifdef _WIN32
		WSACleanup();
#elif __linux__
		close(retsock);
#endif
		exit(10);
	}

	/**
	 * Bind the socket to all addresses on our system and a specific port
	 */
	struct sockaddr_in local = {0};
	memset((char *) &local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	if( bind(retsock, (struct sockaddr*)&local, sizeof(local)) )
	{
		perror("Failed to bind socket!");
#ifdef _WIN32
		WSACleanup();
#elif __linux__
		close(retsock);
#endif
		exit(4);
	}

	/**
	 * Join a multicast group
	 * If we do not do this we wont receive or accept any multicast packets on this socket
	 */
	struct ip_mreq group_req; //struct for multicast group requests
	inet_pton(AF_INET, group.c_str(), &group_req.imr_multiaddr.s_addr);
	inet_pton(AF_INET, ip.c_str(), &group_req.imr_interface.s_addr);

	if(setsockopt(retsock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group_req, sizeof(group_req)) < 0)
	{
		perror("Failed to join multicast group!");
#ifdef _WIN32
		WSACleanup();
#elif __linux__
		close(retsock);
#endif
		exit(5);
	}

	return retsock;
}


void closeReceiver(){
#ifdef _WIN32
		WSACleanup();
#elif __linux__
		close(rsock);
#endif
}

