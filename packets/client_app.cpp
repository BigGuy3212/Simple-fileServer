#include<kamNet/kamNet.h>
#include"Client.h"
#include<iostream>
#include<fstream>
#include<cstddef>

using namespace kamNet;

const char* ip = "127.0.0.1";
const unsigned short port = 4800;

const char* filePath = "C:/Users/kamil/Downloads/image.jpg";

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