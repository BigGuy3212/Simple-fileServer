#include<kamNet/kamNet.h>
#include<iostream>
#include<fstream>
#include"server.h"

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

/*bool processPacket(Packet& packet)
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
}*/

int main() {
	Server server;

	if (server.init(IPEndpoint(ip_address, port)))
	{
		while (true)
		{
			server.Frame();
		}
	}

	Network::shutdown();
	system("pause");
	return 0;
}