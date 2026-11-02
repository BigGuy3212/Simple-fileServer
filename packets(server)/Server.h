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
	Socket listeningSocket;
	std::vector<TCPconnection> connections;
	std::vector<WSAPOLLFD> masterFD;
};