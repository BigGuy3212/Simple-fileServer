#include<kamNet/kamNet.h>
#include<iostream>
#include<fstream>

using namespace kamNet;
using namespace std::filesystem;

const char* ip_address = "0.0.0.0";
unsigned short port = 4800;
unsigned int backflow = 5;

std::string newest_file;
std::string buf;

std::fstream file;
void write_to_file(std::string& data)
{
	if (!file.is_open())
	{
		file.open(newest_file, std::ios::out | std::ios::binary);
	}

	if (file.is_open())
	{
		file.write(data.data(), data.size());
	}
	else
	{
		std::cout << "file not open" << std::endl;
	}
}

bool processPacket(Packet& packet)
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
	}

	default:
		return false;
	}
	return true;
}

int main() {
	if (Network::init())
	{
		std::cout << "Winsock API started up succesfully!\n";

		Socket socket;
		
		if (socket.CreateSocket() == KResult::K_Sucess)
		{
			std::cout << "Socket created sucessfully!\n";
			if (socket.Listen(IPEndpoint(ip_address, port), backflow) == KResult::K_Sucess)
			{
				std::cout << "Socket succesfully listening on port " << port << "!\n";
				Socket newSocket;
				if (socket.Accept(newSocket) == KResult::K_Sucess)
				{
					std::cout << "New connection Accepted!\n";

					std::cout << sizeof(packetType) << std::endl;

					Packet packet;

					
					while (true)
					{

						KResult result = newSocket.Recv(packet);
						if (result != KResult::K_Sucess)
						{
							std::cout << "stopped reciving packets!\n";
							break;
						}

						if (!processPacket(packet))
						{
							break;	
						}
					}

					newSocket.CloseSocket();
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
			socket.CloseSocket();
		}
		else
		{
			std::cerr << "Failed to create socket!\n";
		}
	}
	Network::shutdown();
	system("pause");
	return 0;
}