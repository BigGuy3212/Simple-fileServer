#include"Socket.h"
#include"SocketOptions.h"
#include"IPEndpoint.h"
#include<assert.h>
#include<iostream>

namespace kamNet
{
	kamNet::Socket::Socket(IPVersion ipversion, SocketHandle handle)
		:ipversion(ipversion), handle(handle)
	{
		assert(ipversion == IPVersion::IPv4 || ipversion == IPVersion::IPv6);

	}

	KResult Socket::CreateSocket()
	{
		assert(ipversion == IPVersion::IPv4 || ipversion == IPVersion::IPv6);

		if (handle != INVALID_SOCKET) {
			return KResult::K_GenericError;
		}

		handle = socket(ipversion == IPVersion::IPv4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);

		if (handle == INVALID_SOCKET)
		{
			int result = WSAGetLastError();
			return KResult::K_GenericError;
		}

		if (SetBlocking(false) != KResult::K_Sucess)
		{
			return KResult::K_GenericError;
		}

		if (SetSocketOption(TCP_NoDelay, TRUE) != KResult::K_Sucess)
		{
			return KResult::K_GenericError;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::CloseSocket()
	{
		if (handle == INVALID_SOCKET)
		{
			return KResult::K_GenericError;
		}

		int result = closesocket(handle);
		if (result != 0)
		{
			int error = WSAGetLastError();


			return KResult::K_GenericError;
		}

		handle = INVALID_SOCKET;
		return KResult::K_Sucess;
	}

	KResult Socket::BindSocket(IPEndpoint ipendpoint)
	{
		assert(ipversion == ipendpoint.GetIpVersion());


		if (ipversion == IPVersion::IPv4)
		{
			sockaddr_in addr = ipendpoint.GetSockAddrIPv4();
			int result = bind(handle, (sockaddr*)&addr, sizeof(sockaddr_in));
			if (result != 0)
			{
				int error = WSAGetLastError();
				return KResult::K_GenericError;
			}
		}
		else
		{
			sockaddr_in6 addr = ipendpoint.GetSockAddrIPv6();
			int result = bind(handle, (sockaddr*)&addr, sizeof(sockaddr_in6));
			if (result != 0)
			{
				int error = WSAGetLastError();
				return KResult::K_GenericError;
			}
		}
		return KResult::K_Sucess;

	}

	KResult Socket::Listen(IPEndpoint endpoint, int backlog)
	{
		if (ipversion == IPVersion::IPv4)
		{
			if (SetSocketOption(SocketOption::IPv6_Only, FALSE) != KResult::K_Sucess)
			{
				return KResult::K_GenericError;
			}
		}

		if (BindSocket(endpoint) != KResult::K_Sucess)
		{
			return KResult::K_GenericError;
		}

		int result = listen(handle, backlog);
		if (result != 0)
		{
			int error = WSAGetLastError();
			return K_GenericError;
		}
		
		return K_Sucess;
	}

	SocketHandle Socket::GetHandle()
	{
		return handle;
	}

	IPVersion Socket::GetIpVersion()
	{
		return ipversion;
	}

	KResult Socket::SetSocketOption(SocketOption option, BOOL value)
	{
		int result = 0;
		switch (option)
		{
		case SocketOption::TCP_NoDelay:
			result = setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (const char*) &value, sizeof(value));
			break;
		case SocketOption::IPv6_Only:
			result = setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&value, sizeof(value));
			break;
		default:
			return KResult::K_GenericError;
		}

		if (result != 0)
		{
			int error = WSAGetLastError();
			return KResult::K_GenericError;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::Accept(Socket& outSocket, IPEndpoint* ipendpoint)
	{
		assert(ipversion == IPVersion::IPv4 || ipversion == IPVersion::IPv6);

		if (ipversion == IPVersion::IPv4)
		{
			sockaddr_in addr = {};
			int len = sizeof(addr);

			SocketHandle acceptedConnectHandle = accept(handle, (sockaddr*)&addr, &len);
			if (acceptedConnectHandle == INVALID_SOCKET)
			{
				int error = WSAGetLastError();
				return KResult::K_GenericError;
			}

			if (ipendpoint != nullptr)
			{
				*ipendpoint = IPEndpoint((sockaddr*)&addr);
			}

			outSocket = Socket(IPVersion::IPv4, acceptedConnectHandle);
		}
		else
		{
			sockaddr_in6 addr = {};
			int len = sizeof(sockaddr_in6);

			SocketHandle acceptedConnectHandle = accept(handle, (sockaddr*)&addr, &len);
			if (acceptedConnectHandle == INVALID_SOCKET)
			{
				int error = WSAGetLastError();
				return KResult::K_GenericError;
			}

			if (ipendpoint != nullptr)
			{
				*ipendpoint = IPEndpoint((sockaddr*)&addr);
			}

			outSocket = Socket(IPVersion::IPv6, acceptedConnectHandle);
		}
		return KResult::K_Sucess;
	}

	KResult Socket::Connect(IPEndpoint endpoint)
	{
		assert(ipversion == endpoint.GetIpVersion());

		int result = 0;
		if (ipversion == IPVersion::IPv4)
		{
			sockaddr_in addr = endpoint.GetSockAddrIPv4();
			result = connect(handle, (sockaddr*)&addr, sizeof(sockaddr_in));
			if (result != 0)
			{
				int error = WSAGetLastError();
				return KResult::K_GenericError;
			}
		}
		else
		{
			sockaddr_in6 addr = endpoint.GetSockAddrIPv6();
			result = connect(handle, (sockaddr*)&addr, sizeof(sockaddr_in6));
			if (result != 0)
			{
				int error = WSAGetLastError();
				return KResult::K_GenericError;
			}
		}
		return KResult::K_Sucess;
	}

	KResult Socket::Send(const void* data, int numberOfBytes, int& bytesSent)
	{
		bytesSent = send(handle, (const char*)data, numberOfBytes, NULL);
		if (bytesSent == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			return KResult::K_GenericError;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::Recv(void* destination, int numberOfBytes, int& bytesRecived)
	{
		bytesRecived = recv(handle, (char*)destination, numberOfBytes, NULL);
		if (bytesRecived == 0)
		{
			return KResult::K_GenericError;
		}
		if (bytesRecived == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			return KResult::K_GenericError;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::SendAll(const void* data, int numberOfBytes)
	{
		int totalBytesSent = 0;

		while (totalBytesSent < numberOfBytes)
		{
			int bytesRamaining = numberOfBytes - totalBytesSent;
			int bytesSent = 0;
			char* bufOffset = (char*)data + totalBytesSent;
			KResult result = Send(bufOffset, bytesRamaining, bytesSent);
			if (result != KResult::K_Sucess)
			{
				return KResult::K_GenericError;
			}
			totalBytesSent += bytesSent;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::RecvAll(void* destination, int numberOfBytes)
	{
		int totalBytesRecived = 0;

		while (totalBytesRecived < numberOfBytes)
		{
			int bytesRamaining = numberOfBytes - totalBytesRecived;
			int bytesRecived = 0;
			char* bufOffset = (char*)destination + totalBytesRecived;
			KResult result = Recv(bufOffset, bytesRamaining, bytesRecived);
			if (result != KResult::K_Sucess)
			{
				return KResult::K_GenericError;
			}
			totalBytesRecived += bytesRecived;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::Send(Packet& packet)
	{
		uint16_t encodedPacketSize = htons(packet.buf.size());
		KResult result = SendAll(&encodedPacketSize, sizeof(uint16_t));
		if (result != KResult::K_Sucess)
		{
			return KResult::K_GenericError;
		}

		result = SendAll(packet.buf.data(), packet.buf.size());
		if (result != KResult::K_Sucess)
		{
			return KResult::K_GenericError;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::Recv(Packet& packet)
	{
		packet.Clear();

		uint16_t encodedSize = 0;
		KResult result = RecvAll(&encodedSize, sizeof(uint16_t));
		if (result != KResult::K_Sucess)
		{
			return KResult::K_GenericError;
		}

		uint16_t buffersize = ntohs(encodedSize);

		if (buffersize > kamNet::MaxPacketSize)
		{
			return KResult::K_GenericError;
		}
		packet.buf.resize(buffersize);
		result = RecvAll(&packet.buf[0], buffersize);
		if (result != KResult::K_Sucess)
		{
			return KResult::K_GenericError;
		}

		return KResult::K_Sucess;
	}

	KResult Socket::SetBlocking(bool isBlocking)
	{
		unsigned long Blocking = 0;
		unsigned long nonBlocking = 1;

		int result = ioctlsocket(handle, FIONBIO, isBlocking ? &Blocking : &nonBlocking);
		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			return KResult::K_GenericError;
		}

		return KResult::K_Sucess;
	}
}