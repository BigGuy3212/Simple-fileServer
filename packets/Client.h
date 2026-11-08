#pragma once

#include<kamNet/kamNet.h>
#include<vector>
#include<fstream>
#include<iostream>
#include<SDL3/SDL.h>

using namespace kamNet;

class Client
{
public:
	bool connect(IPEndpoint ip);
	bool IsConnected();
	bool Frame();
	bool processPacket(std::shared_ptr<Packet> packet);
	void sendFile(std::string path);
	void downloadFile(std::string path);
	bool WindowInit();
	void WindowFrame();
	bool IsRunning();
	void draw();
private:
	void writeToFile(std::string& data);
	bool isConnected = false;
	bool runing = false;
	Socket socket;
	WSAPOLLFD masterFD;
	WSAPOLLFD useFD;
	std::string latestFile;
	std::string buf;
	std::ofstream file;
protected:
	TCPconnection connection;
	void CloseConnection(std::string reason);
	SDL_Window* window;
	SDL_Surface* surface;
	const SDL_PixelFormatDetails* format;
};