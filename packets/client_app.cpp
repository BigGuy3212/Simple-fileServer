#include<kamNet/kamNet.h>
#include"Client.h"
#include<iostream>
#include<fstream>
#include<cstddef>

using namespace kamNet;
using namespace std;

const char* ip = "127.0.0.1";
const unsigned short port = 4800;

const char* filePath = "C:/Users/kamil/Downloads/image.jpg";

unsigned int packetDelay = 0;

KResult sendFile(Socket& socket, const char* filepath)
{
	int bytesRead = 0;

	std::ifstream file(filepath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "FUCK" << std::endl;
	}

	std::streamsize size = file.tellg();

	std::vector<byte> bytes(size);

	file.seekg(0, std::ios::beg);

	file.read(reinterpret_cast<char*>(bytes.data()), size);

	const char* buf = reinterpret_cast<const char*>(bytes.data());
	unsigned char packet_buf[40];
	std::string str_buf;

	for (int i = 0; size >= bytesRead; i++)
	{
		Packet packet(PT_file); 
		
		for (int j = 0; j < 40; j++)
		{
			packet_buf[j] = buf[bytesRead];

			bytesRead++;
		}
		
		str_buf.assign(reinterpret_cast<char*>(packet_buf), sizeof(packet_buf));
		std::cout << str_buf << std::endl;
		packet << str_buf;

		socket.Send(packet);

		Sleep(packetDelay);
	}

	file.close();

	str_buf.clear();
	bytes.clear();

	return KResult::K_Sucess;
}

int main() {
	Client client;

	if(client.connect(IPEndpoint(ip, port)))
	{
		while (client.IsConnected())
		{
			client.Frame();
		}
	}

	Network::shutdown();
	system("pause");

	return 0;
}