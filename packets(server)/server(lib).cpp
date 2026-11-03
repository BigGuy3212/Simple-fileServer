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
	useFD = masterFD;

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

		for (int i = useFD.size() - 1; i >= 1; i--)
		{
			int connectionIndex = i - 1;

			TCPconnection& connection = connections[connectionIndex];

			if (useFD[i].revents & POLLERR)
			{
				CloseConnection(connectionIndex, "Poll error");

				continue;
			}

			if (useFD[i].revents & POLLHUP)
			{
				CloseConnection(connectionIndex, "Poll hangup");

				continue;
			}

			if (useFD[i].revents & POLLNVAL)
			{
				CloseConnection(connectionIndex, "Invalid socket");

				continue;
			}

			if (useFD[i].revents & POLLRDNORM)
			{

				int bytesRecived = 0;

				if (connection.task == PacketTask::ProcessPacketSize)
				{
					bytesRecived = recv(useFD[i].fd, (char*) & connection.packetSize + connection.extractionOffset, sizeof(uint16_t) - connection.extractionOffset, 0);
				}
				else
				{
					bytesRecived = recv(useFD[i].fd, (char*)&connection.buffer + connection.extractionOffset, connection.packetSize - connection.extractionOffset, 0);
				}

				

				if (bytesRecived == 0)
				{
					CloseConnection(connectionIndex, "Recv == 0");

					continue;
				}

				if (bytesRecived == SOCKET_ERROR)
				{
					int error = WSAGetLastError();
					if (error != WSAEWOULDBLOCK)
					{
						CloseConnection(connectionIndex, "Recv < 0");
						continue;
					}

				}

				if (bytesRecived > 0)
				{
					connection.extractionOffset += bytesRecived;
					if (connection.task == PacketTask::ProcessPacketSize)
					{
						if (connection.extractionOffset == sizeof(uint16_t))
						{
							connection.packetSize = ntohs(connection.packetSize);
							if (connection.packetSize > kamNet::MaxPacketSize)
							{
								CloseConnection(connectionIndex, "Packet too large!");
								continue;
							}
							connection.extractionOffset = 0;
							connection.task = PacketTask::ProcessPacketContents;
						}
					}
					else
					{
						if (connection.extractionOffset == connection.packetSize)
						{
							Packet packet;

							packet.buf.resize(connection.packetSize);
							memcpy(&packet.buf[0], &connection.buffer, connection.packetSize);

							if (!ProcessPacket(packet))
							{
								CloseConnection(connectionIndex, "Failed to process packet!\n");
							}

							connection.packetSize = 0;
							connection.extractionOffset = 0;
							connection.task = PacketTask::ProcessPacketSize;
						}
					}

					std::cout << connection.toString() << " - Message size: " << bytesRecived << std::endl;
				}
			}
		}
	}
}

void Server::CloseConnection(int connectionIndex, std::string reason)
{
	TCPconnection& connection = connections[connectionIndex];
	std::cout << "[" << reason <<"] - Connection lost: " << connection.toString() << std::endl;
	masterFD.erase(masterFD.begin() + (connectionIndex + 1));
	useFD.erase(useFD.begin() + (connectionIndex + 1));
	connection.Close();

	connections.erase(connections.begin() + connectionIndex);
}

bool Server::ProcessPacket(Packet& packet)
{
	switch (packet.GetPacketType())
	{
	case packetType::PT_ChatMessage:
	{
		std::string chatmessage;
		packet >> chatmessage;
		std::cout << "Chat Message: " << chatmessage << std::endl;
		break;
	}

	case packetType::PT_IntegerArray:
	{
		uint32_t arraySize = 0;
		packet >> arraySize;
		std::cout << "Array Size: " << arraySize << std::endl;
		for (uint32_t i = 0; i < arraySize; i++)
		{
			uint32_t element = 0;
			packet >> element;
			std::cout << "Element[" << i << "] - " << element << std::endl;
		}
		break;
	}
	case packetType::PT_Save:
	{
		/*std::string request;
		packet >> request;

		newest_file = "C:/Users/kamil/Downloads/" + request;

		std::ofstream newfile("C:/Users/kamil/Downloads/" + request, std::ios::binary);

		file.open(newest_file, std::ios::out | std::ios::binary);

		break;
	}

	case packetType::PT_file:
	{
		packet >> buf;

		std::cout << buf << std::endl;

		write_to_file(buf);

		buf.clear();

		break;*/
	}

	default:
		std::cout << "Unknown Packet type!" << packet.GetPacketType() << std::endl;
		return false;
	}
	return true;
}