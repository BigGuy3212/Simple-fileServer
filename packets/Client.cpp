#include"Client.h"
#include<iostream>
#include<sstream>
#include<string>

using namespace kamNet;

int packetsRecived = 0;
int packetsSent = 0;

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


					std::shared_ptr<Packet> helloPacket = std::make_shared<Packet>(packetType::PT_info);
					*helloPacket << std::string("Hello from client\n");

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
	case packetType::PT_info:
	{
		std::string chatmessage;
		*packet >> chatmessage;
		std::cout << "INFO: " << chatmessage << std::endl;
		break;
	}
	case packetType::PT_upload:
	{
		std::string request;
		*packet >> request;

		latestFile = "C:/Users/kamil/Downloads/" + request;

		file = std::ofstream("C:/Users/kamil/Downloads/" + request, std::ios::binary);

		break;
	}

	case packetType::PT_file:
	{
		*packet >> buf;

		writeToFile(buf);

		packetsRecived++;

		break;
	}

	case packetType::PT_fileEnd:
	{
		file.close();

		std::cout << "Recived " << packetsRecived << " packets" << std::endl;

		packetsRecived = 0;
		
		break;
	}

	case packetType::PT_serverInfo:
	{
		std::string info;

		*packet >> info;

		std::stringstream stringStream(info);

		std::string temp;

		std::vector<std::string> parsedInfo;

		while (std::getline(stringStream, temp, ','))
		{
			parsedInfo.push_back(temp);
		}

		std::cout << "Server Info: \n";
		std::cout << "IP: " << parsedInfo[0] << std::endl;
		std::cout << "Port: " << parsedInfo[1] << std::endl;
		std::cout << "Server name: " << parsedInfo[2] << std::endl;

		break;
	}

	default:
		std::cout << "Unknown packet type - " << packet->GetPacketType() << std::endl;
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

void Client::sendFile(std::string path)
{
	std::cout << "sending file...\n";

	std::shared_ptr<Packet> request = std::make_shared<Packet>(packetType::PT_upload);
	*request << std::string("image2.jpg");

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

		str_buf.assign(reinterpret_cast<char*>(packet_buf), sizeof(str_buf));
		*packet << str_buf;

		Sleep(1);
		
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

	packetsSent = 0;
}

void Client::writeToFile(std::string& data)
{
	if (file.is_open())
	{
		file.write(data.data(), data.size());
	}
}

void Client::downloadFile(std::string path)
{
	std::shared_ptr<Packet> download = std::make_shared<Packet>(packetType::PT_download);
	*download << std::string(path);

	connection.pm_outgoing.Append(download);
}