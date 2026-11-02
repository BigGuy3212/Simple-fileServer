#pragma once

#include "SocketHandler.h"
#include "KResult.h"
#include "IPVersion.h"
#include "SocketOptions.h"
#include "IPEndpoint.h"
#include "constants.h"
#include "packet.h"
#include<WS2tcpip.h>
#include"raw_packet.h"

namespace kamNet
{
	class Socket
	{
	public:
		Socket(IPVersion ipversion = IPVersion::IPv4, SocketHandle = INVALID_SOCKET);
		SocketHandle GetHandle();
		IPVersion GetIpVersion();
		KResult BindSocket(IPEndpoint ipendpoint);
		KResult CreateSocket();
		KResult CloseSocket();
		KResult Listen(IPEndpoint ipendpoint, int backlog = 5);
		KResult Accept(Socket& outSocket, IPEndpoint* ipendpoint = nullptr);
		KResult Connect(IPEndpoint endpoint);
		KResult Send(const void* data, int numberOfBytes, int& bytesSent);
		KResult Recv(void* destination, int numberOfBytes, int& bytesRecived);
		KResult SendAll(const void* data, int numberOfBytes);
		KResult RecvAll(void* destination, int numberOfBytes);
		KResult Send(Packet& packet);
		KResult Recv(Packet& packet);
		KResult SetBlocking(bool isBlocking);
	private:
		KResult SetSocketOption(SocketOption option, BOOL value);
		IPVersion ipversion = IPVersion::IPv4;
		SocketHandle handle = INVALID_SOCKET;
	};
}