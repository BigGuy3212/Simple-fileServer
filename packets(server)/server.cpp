#include<kamNet/kamNet.h>
#include<iostream>
#include"server.h"
#include<thread>
#include<windows.h>
#include<wininet.h>

#pragma comment(lib, "wininet.lib")

using namespace kamNet;

const char* ip_address = "0.0.0.0";
unsigned short port = 4800;

std::string GetExternalIpAddress()
{
	std::string Address = "";

	HINTERNET hInternett = InternetOpenA("IPFecher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternett)
		return "OpenInternett failed";

	HINTERNET hUrl = InternetOpenUrlA(hInternett, "https://api.ipify.org", NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (!hUrl) {
		InternetCloseHandle(hInternett);
		return "Error: InternetOpenUrl failed.";
	}

	char buf[17];
	DWORD bytesRead = 0;

	if (InternetReadFile(hUrl, buf, sizeof(buf) - 1, &bytesRead))
	{
		if (bytesRead > 0)
		{
			buf[bytesRead] = (char)"\0";

			const char* pBuf = buf;

			Address.resize(16);

			for (int i = 0; i < 16; i++)
			{
				if (pBuf[i] != (char)"\0")
				{
					Address[i] = pBuf[i];
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		return "InternetReadFile failed";
	}

	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternett);

	return Address;
}


int main() {
	
	Server server;

	ServerInfo info = {};

	info.ip = GetExternalIpAddress();
	info.port = port;
	info.name = "Local-Server";
	
	if (server.init(IPEndpoint(ip_address, port), info))
	{
		std::thread network([&server](void) {
			while (true)
			{
				server.Frame();
			}
		});
		
		while (true)
		{
			
		}
		
	}

	Network::shutdown();
	system("pause");
	return 0;
}