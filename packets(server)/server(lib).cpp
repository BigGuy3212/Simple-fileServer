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
	WSAPOLLFD listeningSocketFD = {};
	listeningSocketFD.fd = listeningSocket.GetHandle();
	listeningSocketFD.events = POLLRDNORM;
	listeningSocketFD.revents = 0;

	if (WSAPoll(&listeningSocketFD, 1, 1) > 0)
	{
		if (listeningSocketFD.revents & POLLRDNORM)
		{
			Socket newSocket;
			IPEndpoint newConnectionEndpoint;
			if (listeningSocket.Accept(newSocket, &newConnectionEndpoint) == KResult::K_Sucess)
			{
				TCPconnection newConnection(newSocket, newConnectionEndpoint);
				std::cout << newConnection.toString() << " - new connection accepted" << std::endl;

				newConnection.Close();
			}
			else
			{
				std::cout << "Failed to accept connection\n";
			}
		}
	}
}