#pragma once

#define WIN32_LEAN_AND_MEAN

#include<WinSock.h>
#include<vector>
#include<iostream>
#include "PacketExeption.h"
#include "Packet_type.h"
#include<stdio.h>

namespace kamNet
{
	class Packet
	{
	public:
		Packet(packetType packettype = packetType::PT_Invalid);
		packetType GetPacketType();
		void AssignPacketType(packetType packettypye);

		void Clear();
		void Append(const void* data, uint32_t size);

		Packet& operator << (uint32_t data);
		Packet& operator >> (uint32_t& data);

		Packet& operator << (const std::string & data);
		Packet& operator >> (std::string& data);

		uint32_t extractionoffset = 0;

		std::vector<char>buf;
	};
}