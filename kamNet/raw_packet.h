#pragma once

#define WIN32_LEAN_AND_MEAN

#include<WinSock.h>
#include<iostream>
#include"PacketExeption.h"
#include"Packet_type.h"
#include<vector>
#include<stdio.h>

namespace kamNet
{
	class raw_packet
	{
	public:
		raw_packet(packetType packettype = packetType::PT_Invalid);
		packetType GetPacketType();
		void AssignPacketType(packetType packettype);

		void Clear();
		void Append(const unsigned char* data, uint32_t size);
		void AppendU(const void* data, uint32_t size);

		raw_packet& operator << (uint32_t data);
		raw_packet& operator >> (uint32_t& data);

		raw_packet& operator << (unsigned char data);
		raw_packet& operator >> (unsigned char& data);

		uint32_t extractionOffset = 0;

		std::vector<unsigned char>buf;
	};
}