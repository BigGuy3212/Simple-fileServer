#pragma once

#include<kamNet/kamNet.h>

using namespace kamNet;

class Client
{
public:
	bool connect(IPEndpoint ip);
	bool IsConnected();
	bool Frame();
private:
	bool isConnected = false;
	Socket socket;
};