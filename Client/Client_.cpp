#include"Client.h"
#include<iostream>
#include<thread>

using namespace kamNet;

const char* ip = "127.0.0.1";
const unsigned short port = 4800;

const char* filePath = "C:/Users/kamil/Downloads/image.jpg";


int main() {
	
	Client client;
	
	if(client.connect(IPEndpoint(ip, port)))
	{
		std::thread update([&client]() {
			while (client.IsConnected())
			{
				client.Frame();
			}
		});

		while (true)
		{
			Sleep(500);
			std::string req;

			std::cout << "action: ";
			std::getline(std::cin, req);

			if (req == "send")
			{
				client.sendFile(filePath);
			}
			else if (req == "download")
			{
				client.downloadFile("urgalurg");
			}
		}
	}

	Network::shutdown();
	system("pause");

	return 0;
}