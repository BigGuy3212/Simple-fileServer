#include"server.h"
#include<iostream>

using namespace kamNet;

bool Server::init(IPEndpoint ip)
{
	masterFD.clear();
	connections.clear();

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
				WSAPOLLFD listeningSocketFD = {};
				listeningSocketFD.fd = listeningSocket.GetHandle();
				listeningSocketFD.events = POLLRDNORM;
				listeningSocketFD.revents = 0;

				masterFD.push_back(listeningSocketFD);

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
	std::vector<WSAPOLLFD> useFD = masterFD;

	if (WSAPoll(useFD.data(), useFD.size(), 1) > 0)
	{
		WSAPOLLFD listeningSocketFD = useFD[0];
		if (listeningSocketFD.revents & POLLRDNORM)
		{
			Socket newSocket;
			IPEndpoint newConnectionEndpoint;
			if (listeningSocket.Accept(newSocket, &newConnectionEndpoint) == KResult::K_Sucess)
			{
				connections.emplace_back(TCPconnection(newSocket, newConnectionEndpoint));
				TCPconnection& newConnection = connections[connections.size() - 1];
				std::cout << newConnection.toString() << " - new connection accepted" << std::endl;
				WSAPOLLFD newConnectionFD = {};
				newConnectionFD.fd = newSocket.GetHandle();
				newConnectionFD.events = POLLRDNORM;
				newConnectionFD.revents = 0;
				masterFD.push_back(newConnectionFD);
			}
			else
			{
				std::cout << "Failed to accept connection\n";
			}
		}

		for (int i = 1; i < useFD.size(); i++)
		{
			int connectionIndex = i - 1;

			TCPconnection& connection = connections[connectionIndex];

			if (useFD[i].revents & POLLERR)
			{
				std::cout << "Poll error occured on: " << connection.toString() << std::endl;
				masterFD.erase(masterFD.begin() + i);
				useFD.erase(useFD.begin() + i);
				connection.Close();

				connections.erase(connections.begin() + connectionIndex);

				i -= 1;
				continue;
			}

			if (useFD[i].revents & POLLHUP)
			{
				std::cout << "Poll error occured on: " << connection.toString() << std::endl;
				masterFD.erase(masterFD.begin() + i);
				useFD.erase(useFD.begin() + i);
				connection.Close();

				connections.erase(connections.begin() + connectionIndex);

				i -= 1;
				continue;
			}

			if (useFD[i].revents & POLLNVAL)
			{
				std::cout << "Invalid socket used on: " << connection.toString() << std::endl;
				masterFD.erase(masterFD.begin() + i);
				useFD.erase(useFD.begin() + i);
				connection.Close();

				connections.erase(connections.begin() + connectionIndex);

				i -= 1;
				continue;
			}

			if (useFD[i].revents & POLLRDNORM)
			{
				char buf[kamNet::MaxPacketSize];

				int bytesRecived;

				bytesRecived = recv(useFD[i].fd, buf, kamNet::MaxPacketSize, 0);

				if (bytesRecived == 0)
				{
					std::cout << "Connection lost on: " << connection.toString() << std::endl;
					masterFD.erase(masterFD.begin() + i);
					useFD.erase(useFD.begin() + i);
					connection.Close();

					connections.erase(connections.begin() + connectionIndex);

					i -= 1;
					continue;
				}

				if (bytesRecived == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					if (error != WSAEWOULDBLOCK)
					{
						std::cout << "Connection lost: " << connection.toString() << std::endl;
						std::cout << error << std::endl;
						masterFD.erase(masterFD.begin() + i);
						useFD.erase(useFD.begin() + i);
						connection.Close();

						connections.erase(connections.begin() + connectionIndex);

						i -= 1;
						continue;
					}

				}

				if (bytesRecived > 0)
				{
					std::cout << connection.toString() << " - Message size: " << bytesRecived << std::endl;
				}
			}
		}
	}
}