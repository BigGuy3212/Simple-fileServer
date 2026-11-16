#include"Client.h"
#include<kamNet/kamNet.h>
#include<iostream>
#include<thread>
#include<SDL3/SDL.h>

using namespace kamNet;

const char* filePath = "C:/Users/kamil/Downloads/image.jpg";

std::atomic<bool> shoudStop(false);

int main() {

	Client client;
	
	if(client.WindowInit())
	{
		std::thread update([&client]() {
			Network::init();
			while (!shoudStop.load())
			{
				if (client.IsConnected())
				{
					client.Frame();
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