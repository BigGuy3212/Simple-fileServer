#include<kamNet/kamNet.h>
#include<iostream>
#include"server.h"

using namespace kamNet;

const char* ip_address = "0.0.0.0";
unsigned short port = 4800;

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