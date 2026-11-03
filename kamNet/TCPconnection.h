#pragma once

#include"Socket.h"

namespace kamNet
{
	enum PacketTask
	{
		ProcessPacketSize,
		ProcessPacketContents
	};

	class TCPconnection
	{
	public:
		TCPconnection(Socket socket, IPEndpoint ipendpoint);
		void Close();
		std::string toString();
		Socket socket;

		PacketTask task = PacketTask::ProcessPacketSize;
		int extractionOffset = 0;
		uint16_t packetSize = 0;
		char buffer[kamNet::MaxPacketSize];
	private:
		IPEndpoint ipendpoint;
		std::string stringRepresentation = "";
	};
}