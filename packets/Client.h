#pragma once

#include<kamNet/kamNet.h>
#include<vector>
#include<fstream>
#include<iostream>

using namespace kamNet;

class Client
{
public:
	bool connect(IPEndpoint ip);
	bool IsConnected();
	bool Frame();
	bool processPacket(std::shared_ptr<Packet> packet);
	void sendFile(std::string path);
private:
	void writeToFile(std::string& data);
	bool isConnected = false;
	Socket socket;
	WSAPOLLFD masterFD;
	WSAPOLLFD useFD;
	std::string latestFile;
	std::string buf;
	std::ofstream file;
protected:
	TCPconnection connection;
	void CloseConnection(std::string reason);
};