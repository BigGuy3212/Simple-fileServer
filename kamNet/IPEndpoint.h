#pragma once

#include "IPVersion.h"
#include<string>
#include<vector>
#include<WinSock2.h>
#include<WS2tcpip.h>

namespace kamNet
{
	class IPEndpoint
	{
	public:
		IPEndpoint(const char* ip, unsigned short port);
		IPEndpoint(sockaddr* addr);
		IPVersion GetIpVersion();
		sockaddr_in GetSockAddrIPv4();
		sockaddr_in6 GetSockAddrIPv6();
		std::vector<uint8_t>GetIpBytes();
		std::string GetHostName();
		std::string GetIpString();
		unsigned short GetPort();
		void Print();
	private:
		IPVersion ipversion = IPVersion::Unknown;
		std::string hostname = "";
		std::string ip_string = "";
		std::vector<uint8_t> ip_bytes;
		unsigned short port = 0;
	};
}