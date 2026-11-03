#pragma once

#include"packet.h"
#include<queue>
#include<memory>

namespace kamNet
{
	enum PacketManagerTask
	{
		ProcessPacketSize,
		ProcessPacketContents
	};

	class PacketManager
	{
	public:
		void Clear();
		bool HasPendingPackets();
		void Append(std::shared_ptr<Packet> p);
		std::shared_ptr<Packet> Retrive();
		void pop();

		uint16_t currentPacketSize = 0;
		int currentPacketExtractionOffset = 0;
		PacketManagerTask currentTask = PacketManagerTask::ProcessPacketSize;
	private:
		std::queue<std::shared_ptr<Packet>> packets;
	};
}