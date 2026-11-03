#pragma once

#include<kamNet/kamNet.h>
#include<vector>

using namespace kamNet;

class Client
{
public:
	bool connect(IPEndpoint ip);
	bool IsConnected();
	bool Frame();
	bool processPacket(std::shared_ptr<Packet> packet);
private:
	bool isConnected = false;
	Socket socket;
	WSAPOLLFD masterFD;
	WSAPOLLFD useFD;
protected:
	TCPconnection connection;
	void CloseConnection(std::string reason);
};