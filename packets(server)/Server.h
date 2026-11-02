#pragma once

#include<kamNet/kamNet.h>

using namespace kamNet;

class Server
{
public:
	bool init(IPEndpoint ip);
	void Frame();

private:
	Socket listeningSocket;
};