#include"server.h"
#include<iostream>
#include<kamNet/kamNet.h>

using namespace kamNet;

int packetsRecived = 0;
int packetsSent = 0;

bool Server::init(IPEndpoint ip, ServerInfo& info)
{
	masterFD.clear();
	connections.clear();

	Serverinfo = info;

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
	for (int i = 0; i < connections.size(); i++)
	{
		if (connections[i].pm_outgoing.HasPendingPackets())
		{
			masterFD[i + 1].events = POLLRDNORM | POLLWRNORM;
		}
	}

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

				std::shared_ptr<Packet> stringPacket = std::make_shared<Packet>(packetType::PT_info);
				*stringPacket << std::string("Welcome to the server\n");

				std::shared_ptr<Packet> serverInfo = std::make_shared<Packet>(packetType::PT_serverInfo);
				*serverInfo << std::string(Serverinfo.ip + "," + std::to_string(Serverinfo.port) + "," + Serverinfo.name);

				newConnection.pm_outgoing.Append(stringPacket);
				newConnection.pm_outgoing.Append(serverInfo);
			}
			else
			{
				std::cout << "Failed to accept connection\n";
			}
		}

		for (int i = useFD.size() - 1; i >= 1; i--)
		{
			connectionIndex = i - 1;

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

				if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
				{
					bytesRecived = recv(useFD[i].fd, (char*) & connection.pm_incoming.currentPacketSize + connection.pm_incoming.currentPacketExtractionOffset, sizeof(uint16_t) - connection.pm_incoming.currentPacketExtractionOffset, 0);
				}
				else
				{
					bytesRecived = recv(useFD[i].fd, (char*)&connection.buffer + connection.pm_incoming.currentPacketExtractionOffset, connection.pm_incoming.currentPacketSize - connection.pm_incoming.currentPacketExtractionOffset, 0);
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
					connection.pm_incoming.currentPacketExtractionOffset += bytesRecived;
					if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
					{
						if (connection.pm_incoming.currentPacketExtractionOffset == sizeof(uint16_t))
						{
							connection.pm_incoming.currentPacketSize = ntohs(connection.pm_incoming.currentPacketSize);
							if (connection.pm_incoming.currentPacketSize > kamNet::MaxPacketSize)
							{
								CloseConnection(connectionIndex, "Packet too large!");
								continue;
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

			if (useFD[i].revents & POLLWRNORM)
			{
				PacketManager& pm = connection.pm_outgoing;

				while (pm.HasPendingPackets())
				{
					if (pm.currentTask == PacketManagerTask::ProcessPacketSize)
					{
						pm.currentPacketSize = pm.Retrive()->buf.size();
						uint16_t bigEndiaPacketSize = htons(pm.currentPacketSize);
						int bytesSent = send(useFD[i].fd, (char*)(&bigEndiaPacketSize) + pm.currentPacketExtractionOffset, sizeof(uint16_t) - pm.currentPacketExtractionOffset, 0);
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

						int bytesSent = send(useFD[i].fd, (char*)(pBuffer) + pm.currentPacketExtractionOffset, pm.currentPacketSize - pm.currentPacketExtractionOffset, 0);
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

				if (!pm.HasPendingPackets())
				{
					masterFD[i].events = POLLRDNORM;
				}
			}
		}
	}

	for (int i = connections.size() - 1; i >= 0; i--)
	{
		while (connections[i].pm_incoming.HasPendingPackets())
		{
			std::shared_ptr<Packet> frontPacket = connections[i].pm_incoming.Retrive();
			if (!ProcessPacket(frontPacket))
			{
				CloseConnection(i, "Failed to process incoming packet!\n");
				break;
			}
			connections[i].pm_incoming.pop();
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

bool Server::ProcessPacket(std::shared_ptr<Packet> packet)
{
	switch (packet->GetPacketType())
	{
	case packetType::PT_info:
	{
		std::string chatmessage;
		*packet >> chatmessage;
		std::cout << "Chat Message: " << chatmessage << std::endl;
		break;
	}
	case packetType::PT_upload:
	{
		std::string request;
		*packet >> request;

		latestFile = "C:/Users/kamil/Downloads/" + request;

		file = std::ofstream(latestFile, std::ios::binary | std::ios::ate);

		std::cout << "Uploading file - " << request << std::endl;

		break;
	}

	case packetType::PT_file:
	{
		*packet >> buf;

		writeToFile(buf);

		packetsRecived++;
		
		break;
	}

	case packetType::PT_download:
	{
		sendFile("C:/Users/kamil/Downloads/image.jpg", connections[connectionIndex]);

		break;
	}

	case packetType::PT_fileEnd:
	{
		file.close();

		std::cout << "Recived " << packetsRecived << " packets" << std::endl;

		packetsRecived = 0;

		break;
	}

	default:
		std::cout << "Unknown Packet type!" << packet->GetPacketType() << std::endl;
		return false;
	}
	return true;
}

void Server::writeToFile(std::string& data)
{
	if (file.is_open())
	{
		file.write(data.data(), data.size());
	}
}

void Server::sendFile(std::string path, TCPconnection& connection)
{
	std::cout << "sending file\n";

	std::shared_ptr<Packet> request = std::make_shared<Packet>(packetType::PT_upload);
	*request << std::string("image3.jpg");

	connection.pm_outgoing.Append(request);

	int bytesRead = 0;

	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Could not open file!" << std::endl;
	}

	std::streamsize size = file.tellg();

	std::vector<std::byte> bytes(size);

	file.seekg(0, std::ios::beg);

	file.read(reinterpret_cast<char*>(bytes.data()), size);

	const char* buf = reinterpret_cast<const char*>(bytes.data());
	unsigned char packet_buf[40];
	std::string str_buf;

	for(int i = 0;size >= bytesRead; i++)
	{
		std::shared_ptr<Packet> packet = std::make_shared<Packet>(packetType::PT_file);

		for (int j = 0; j < 40; j++)
		{
			packet_buf[j] = buf[bytesRead];

			bytesRead++;
		}

		str_buf.assign(reinterpret_cast<char*>(packet_buf), sizeof(packet_buf));
		*packet << str_buf;

		connection.pm_outgoing.Append(packet);

		packetsSent++;
	}

	file.close();

	str_buf.clear();
	bytes.clear();

	std::shared_ptr<Packet> endFile = std::make_shared<Packet>(packetType::PT_fileEnd);
	*endFile << std::string("end of file");

	connection.pm_outgoing.Append(endFile);

	std::cout << "Sent " << packetsSent << " packets" << std::endl;
}