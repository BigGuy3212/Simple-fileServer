#include"Client.h"
#include<kamNet/kamNet.h>
#include<iostream>
#include<thread>
#include<SDL3/SDL.h>

using namespace kamNet;

const char* ip = "127.0.0.1";
const unsigned short port = 4800;

const char* filePath = "C:/Users/kamil/Downloads/image.jpg";

std::atomic<bool> shoudStop(false);

int main() {

	Client client;
	
	if(client.WindowInit())
	{
		std::thread update([&client]() {
			if (!shoudStop.load())
			{
				if (client.connect(IPEndpoint(ip, port)))
				{
					if (client.IsConnected())
					{
						while (client.IsConnected())
						{
							client.Frame();
						}
					}
				}
			}
		});

		while (client.IsRunning())
		{
			client.WindowFrame();
		}

		client.shutdown();

		shoudStop.store(true);

		update.join();
		
	}

	Network::shutdown();

	return 0;
}