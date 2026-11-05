#pragma once
#include<iostream>

namespace kamNet
{
	typedef struct ServerInfo
	{
		std::string ip;
		unsigned short port;
		std::string name;
	};
}