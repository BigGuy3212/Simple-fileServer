#include"server.h"
#include<iostream>

using namespace kamNet;

bool Server::init(IPEndpoint ip)
{
	if (Network::init())
	{
		std::cout << "Winsock API started up succesfully!\n";

		listeningSocket = Socket(ip.GetIpVersion());

		if (listeningSocket.CreateSocket() == KResult::K_Sucess)
		{
			std::cout << "Socket created sucessfully!\n";
			if (listeningSocket.Listen(ip) == KResult::K_Sucess)
			{
				std::cout << "Socket succesfully listening on port " << ip.GetPort() << "!\n";
				return true;
			}
			else
			{
				std::cerr << "New connection Denied\n";
			}
		}
		else
		{
			std::cerr << "Socket failed to listen on port!\n";
		}
		listeningSocket.CloseSocket();
	}
	else
	{
			std::cerr << "Failed to create socket!\n";
	}

	return false;
}

void Server::Frame()
{
	Socket newSocket;
	if (listeningSocket.Accept(newSocket) == KResult::K_Sucess)
	{
		std::cout << "New connection Accepted!\n";

		newSocket.CloseSocket();

		}
}