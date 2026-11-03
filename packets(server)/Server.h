#pragma once

#include<kamNet/kamNet.h>
#include<vector>

using namespace kamNet;

class Server
{
public:
	bool init(IPEndpoint ip);
	void Frame();
private:
	void CloseConnection(int connectionIndex, std::string reason);
	bool ProcessPacket(Packet& packet);
	Socket listeningSocket;
	std::vector<TCPconnection> connections;
	std::vector<WSAPOLLFD> masterFD;
	std::vector<WSAPOLLFD> useFD;
};