#include "TCPconnection.h"

namespace kamNet
{
	TCPconnection::TCPconnection(Socket socket, IPEndpoint ipendpoint)
		:socket(socket), ipendpoint(ipendpoint)
	{
		stringRepresentation = "[" + ipendpoint.GetIpString();
		stringRepresentation += ":" + std::to_string(ipendpoint.GetPort()) + "]";
	}

	void TCPconnection::Close()
	{
		socket.CloseSocket();
	}

	std::string TCPconnection::toString()
	{
		return stringRepresentation;
	}
}