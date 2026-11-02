#include<kamNet/kamNet.h>
#include<iostream>
#include<fstream>
#include<cstddef>

using namespace kamNet;
using namespace std;

const char* ip = "127.0.0.1";

const char* filePath = "C:/Users/kamil/Downloads/image.jpg";

unsigned int packetDelay = 0;

KResult sendSTR(Socket& socket)
{
	Packet StrinPacket(packetType::PT_ChatMessage);
	StrinPacket << std::string("The string packet!");

	KResult result;
	result = socket.Send(StrinPacket);
	
	std::cout << "sent packet" << std::endl;
	
	return result;
}

KResult sendINT(Socket& socket)
{
	Packet integerPacket(packetType::PT_IntegerArray);
	uint32_t arraySize = 6;
	uint32_t integerArray[6] = { 2, 6, 3, 2, 43, 2 };
	integerPacket << arraySize;
	for (auto integer : integerArray)
	{
		integerPacket << integer;
	}
	KResult result;
	result = socket.Send(integerPacket);
	
	std::cout << "sent packet" << std::endl;
	
	return result;
}

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
	if (Network::init())
	{
		std::cout << "Winsock API started up successfully!\n";

		Socket socket;
		if (socket.CreateSocket() == KResult::K_Sucess)
		{

			std::cout << "Socket created sucessfully!\n";

			if (socket.Connect(IPEndpoint(ip, 4800)) == KResult::K_Sucess)
			{
				std::cout << "Succesfully connected to the server on "<< ip << "\n";
				
				Packet filename(PT_Save);

				filename << std::string("image2.jpg");

				socket.Send(filename);

				KResult result = sendFile(socket, filePath);
				if (result != KResult::K_Sucess)
				{
					std::cout << "fuck" << std::endl;
				}
				else
				{
					std::cout << "sendt image" << std::endl;
				}
				/*while (true)
				{
					KResult result;

					if (rand() % 2 == 0)
					{
						result = sendSTR(socket);
					}
					else
					{
						result = sendINT(socket);
					}

					if (result != KResult::K_Sucess)
					{
						std::cout << "stopped sending packets!\n";
						break;
					}
					
					Sleep(500);
				}*/
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
	Network::shutdown();
	system("pause");

	return 0;
}