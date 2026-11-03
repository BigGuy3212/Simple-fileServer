#pragma once

#include"Socket.h"
#include"PacketManager.h"

namespace kamNet
{
	class TCPconnection
	{
	public:
		TCPconnection(Socket socket, IPEndpoint ipendpoint);
		void Close();
		std::string toString();
		Socket socket;

		PacketManager pm_incoming;
		PacketManager pm_outgoing;
		char buffer[kamNet::MaxPacketSize];
	private:
		IPEndpoint ipendpoint;
		std::string stringRepresentation = "";
	};
}