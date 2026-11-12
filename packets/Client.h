#pragma once

#include<kamNet/kamNet.h>
#include<vector>
#include<fstream>
#include<iostream>
#include<SDL3/SDL.h>
#include<RmlUi/Core/Context.h>
#include<RmlUi/Core/Core.h>
#include<RmlUi/Core/Log.h>
#include "RmlUi_Backend.h"
#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_SDL_GPU.h"
#include<RmlUi/Debugger.h>
#include<RmlUi/Core.h>
#include"Shell.h"
#include<functional>
#include<windows.h>
#include<shobjidl.h>

#ifndef RMLUI_BACKEND_SDL_GPU_DEBUG
#define RMLUI_BACKEND_SDL_GPU_DEBUG false
#endif

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
	void shutdown();
private:
	void writeToFile(std::string& data);
	void LoadUI();
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
	Rml::Context* context = nullptr;
	Rml::UniquePtr<TextInputMethodEditor_SDL> textInput = nullptr;
	Rml::ElementDocument* document;
	std::string filepath;
	std::wstring OpenFileExplorer();
};

class EventListener : public Rml::EventListener
{
public:
	EventListener(std::function<void()> func);

	void ProcessEvent(Rml::Event& event) override
	{
		if (event.GetId() == Rml::EventId::Click)
		{
			function();
		}
	}

	std::function<void()> function;
};