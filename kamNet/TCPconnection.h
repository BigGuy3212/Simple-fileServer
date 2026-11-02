#pragma once

#include"Socket.h"

namespace kamNet
{
	class TCPconnection
	{
	public:
		TCPconnection(Socket socket, IPEndpoint ipendpoint);
		void Close();
		std::string toString();
		Socket socket;
	private:
		IPEndpoint ipendpoint;
		std::string stringRepresentation = "";
	};
}