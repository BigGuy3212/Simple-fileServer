#include"Network.h"
#include<iostream>

#pragma comment(lib, "ws2_32.lib")

bool kamNet::Network::init()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		std::cerr << "Failed to start up winsock API!";
		return false;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		std::cerr << "Could not find winsock dll with version 2.2!";
		return false;
	}
	
	return true;
}

void kamNet::Network::shutdown()
{
	WSACleanup();
}