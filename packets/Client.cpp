#include"Client.h"
#include<iostream>

using namespace kamNet;

bool Client::connect(IPEndpoint ip)
{
	isConnected = false;
	if (Network::init())
	{
		std::cout << "Winsock API started up successfully!\n";

		socket = Socket(ip.GetIpVersion());
		if (socket.CreateSocket() == KResult::K_Sucess)
		{
			if (socket.SetBlocking(true) != KResult::K_Sucess)
				return false;

			std::cout << "Socket created sucessfully!\n";

			if (socket.Connect(ip) == KResult::K_Sucess)
			{
				if (socket.SetBlocking(false) == KResult::K_Sucess)
				{
					std::cout << "Succesfully connected to the server on " << ip.GetIpString() << "\n";

					connection = TCPconnection(socket, ip);
					masterFD.fd = connection.socket.GetHandle();
					masterFD.events = POLLRDNORM;
					masterFD.revents = 0;

					std::shared_ptr<Packet> helloPacket = std::make_shared<Packet>(packetType::PT_ChatMessage);
					*helloPacket << std::string("Hello from client");

					connection.pm_outgoing.Append(helloPacket);

					isConnected = true;
					return true;
				}
			}
			else
			{
				std::cerr << "Failed to connect to the server!\n";
			}
			socket.CloseSocket();
		}
		else
		{
			std::cerr << "Failed to create socket!\n";
		}
	}
	return false;
}

bool Client::IsConnected()
{
	return isConnected;
}

bool Client::Frame()
{
	if (connection.pm_outgoing.HasPendingPackets())
	{
		masterFD.events = POLLRDNORM | POLLWRNORM;
	}

	useFD = masterFD;

	if (WSAPoll(&useFD, 1, 1) > 0)
	{

		if (useFD.revents & POLLERR)
		{
			CloseConnection("Poll error");

			return false;
		}

		if (useFD.revents & POLLHUP)
		{
			CloseConnection("Poll hangup");

			return false;
		}

		if (useFD.revents & POLLNVAL)
		{
			CloseConnection("Invalid socket");

			return false;
		}

		if (useFD.revents & POLLRDNORM)
		{

			int bytesRecived = 0;

			if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
			{
				bytesRecived = recv(useFD.fd, (char*)&connection.pm_incoming.currentPacketSize + connection.pm_incoming.currentPacketExtractionOffset, sizeof(uint16_t) - connection.pm_incoming.currentPacketExtractionOffset, 0);
			}
			else
			{
				bytesRecived = recv(useFD.fd, (char*)&connection.buffer + connection.pm_incoming.currentPacketExtractionOffset, connection.pm_incoming.currentPacketSize - connection.pm_incoming.currentPacketExtractionOffset, 0);
			}

			if (bytesRecived == 0)
			{
				CloseConnection("Recv == 0");

				return false;
			}

			if (bytesRecived == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				if (error != WSAEWOULDBLOCK)
				{
					CloseConnection("Recv < 0");
					return false;
				}

			}

			if (bytesRecived > 0)
			{
				connection.pm_incoming.currentPacketExtractionOffset += bytesRecived;
				if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
				{
					if (connection.pm_incoming.currentPacketExtractionOffset == sizeof(uint16_t))
					{
						connection.pm_incoming.currentPacketSize = ntohs(connection.pm_incoming.currentPacketSize);
						if (connection.pm_incoming.currentPacketSize > kamNet::MaxPacketSize)
						{
							CloseConnection("Packet too large!");
							return false;
						}
						connection.pm_incoming.currentPacketExtractionOffset = 0;
						connection.pm_incoming.currentTask = PacketManagerTask::ProcessPacketContents;
					}
				}
				else
				{
					if (connection.pm_incoming.currentPacketExtractionOffset == connection.pm_incoming.currentPacketSize)
					{
						std::shared_ptr<Packet> packet = std::make_shared<Packet>();

						packet->buf.resize(connection.pm_incoming.currentPacketSize);
						memcpy(&packet->buf[0], &connection.buffer, connection.pm_incoming.currentPacketSize);

						connection.pm_incoming.Append(packet);

						connection.pm_incoming.currentPacketSize = 0;
						connection.pm_incoming.currentPacketExtractionOffset = 0;
						connection.pm_incoming.currentTask = PacketManagerTask::ProcessPacketSize;
					}
				}
			}
		}

		if (useFD.revents & POLLWRNORM)
		{
			PacketManager& pm = connection.pm_outgoing;

			while (pm.HasPendingPackets())
			{
				if (pm.currentTask == PacketManagerTask::ProcessPacketSize)
				{
					pm.currentPacketSize = pm.Retrive()->buf.size();
					uint16_t bigEndiaPacketSize = htons(pm.currentPacketSize);
					int bytesSent = send(useFD.fd, (char*)(&bigEndiaPacketSize) + pm.currentPacketExtractionOffset, sizeof(uint16_t) - pm.currentPacketExtractionOffset, 0);
					if (bytesSent > 0)
					{
						pm.currentPacketExtractionOffset += bytesSent;
					}

					if (pm.currentPacketExtractionOffset == sizeof(uint16_t))
					{
						pm.currentPacketExtractionOffset = 0;
						pm.currentTask = PacketManagerTask::ProcessPacketContents;
					}
					else
					{
						break;
					}
				}
				else
				{
					char* pBuffer = &pm.Retrive()->buf[0];

					int bytesSent = send(useFD.fd, (char*)(pBuffer)+pm.currentPacketExtractionOffset, pm.currentPacketSize - pm.currentPacketExtractionOffset, 0);
					if (bytesSent > 0)
					{
						pm.currentPacketExtractionOffset += bytesSent;
					}

					if (pm.currentPacketExtractionOffset == pm.currentPacketSize)
					{
						pm.currentPacketExtractionOffset = 0;
						pm.currentTask = PacketManagerTask::ProcessPacketSize;
						pm.pop();
					}
					else
					{
						break;
					}
				}
			}
			if (connection.pm_outgoing.HasPendingPackets())
			{
				masterFD.events = POLLRDNORM;
			}
		}
	}
	while (connection.pm_incoming.HasPendingPackets())
	{
		std::shared_ptr<Packet> frontPacket = connection.pm_incoming.Retrive();
		if (!processPacket(frontPacket))
		{
			CloseConnection("Failed to process incoming packet!\n");
			return false;
		}
		connection.pm_incoming.pop();
	}
}

bool Client::processPacket(std::shared_ptr<Packet> packet)
{
	switch (packet->GetPacketType())
	{
	case packetType::PT_ChatMessage:
	{
		std::string chatmessage;
		*packet >> chatmessage;
		std::cout << "Chat Message: " << chatmessage << std::endl;
		break;
	}

	case packetType::PT_IntegerArray:
	{
		uint32_t arraySize = 0;
		*packet >> arraySize;
		std::cout << "Array Size: " << arraySize << std::endl;
		for (uint32_t i = 0; i < arraySize; i++)
		{
			uint32_t element = 0;
			*packet >> element;
			std::cout << "Element[" << i << "] - " << element << std::endl;
		}
		break;
	}
	/*case packetType::PT_Save:
	{
		std::string request;
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

		break;
	}*/

	default:
		return false;
	}
	return true;
}

void Client::CloseConnection(std::string reason)
{
	masterFD.fd = 0;
	isConnected = false;
	connection.Close();
}