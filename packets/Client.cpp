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

SDL_Window* window = SDL_CreateWindow("s1mple", 800, 800, 0);
SDL_GPUDevice* GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, NULL);

bool claim = SDL_ClaimWindowForGPUDevice(GPU, window);

int main() {

	Client client(GPU, window);
	
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

		shoudStop.store(true);

		update.join();
		
	}

	Network::shutdown();

	return 0;
}