#pragma once

#include<kamNet/kamNet.h>
#include<vector>
#include<fstream>

using namespace kamNet;

class Server
{
public:
	bool init(IPEndpoint ip);
	void Frame();
	void sendFile(std::string path, TCPconnection& connection);
private:
	void CloseConnection(int connectionIndex, std::string reason);
	bool ProcessPacket(std::shared_ptr<Packet> packet);
	void writeToFile(std::string& data);
	int connectionIndex = 0;
	Socket listeningSocket;
	std::vector<TCPconnection> connections;
	std::vector<WSAPOLLFD> masterFD;
	std::vector<WSAPOLLFD> useFD;
	std::string latestFile;
	std::string buf;
	std::ofstream file;
};