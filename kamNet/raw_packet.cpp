#include"raw_packet.h"
#include"constants.h"

namespace kamNet
{
	void raw_packet::Clear()
	{
		buf.resize(sizeof(packetType));
		AssignPacketType(packetType::PT_Invalid);
		extractionOffset = sizeof(packetType);
	}

	void raw_packet::AssignPacketType(packetType packettype)
	{
		packetType* pPackettype = reinterpret_cast<packetType*>(&buf[0]);
		*pPackettype = static_cast<packetType>(htons(packettype));
	}

	packetType raw_packet::GetPacketType()
	{
		packetType* packettypePtr = reinterpret_cast<packetType*>(&buf[0]);
		return static_cast<packetType>(ntohs(*packettypePtr));
	}

	raw_packet::raw_packet(packetType packettype)
	{
		Clear();
		AssignPacketType(packettype);
	}

	void raw_packet::Append(const unsigned char* data, uint32_t size)
	{
		if ((buf.size() + size) > kamNet::MaxPacketSize)
		{
			throw PacketExeption("[Packet::Append(const void* data, uint32_t size)] - Packet exeded max packet size!");
		}
		buf.insert(buf.end(), data, data + size);
	}

	void raw_packet::AppendU(const void* data, uint32_t size)
	{
		if ((buf.size() + size) > kamNet::MaxPacketSize)
		{
			throw PacketExeption("[Packet::Append(const void* data, uint32_t size)] - Packet exeded max packet size!");
		}
		buf.insert(buf.end(), (unsigned char*)data, (unsigned char*)data + size);
	}

	raw_packet& raw_packet::operator << (uint32_t data)
	{
		data = htonl(data);
		AppendU(&data, sizeof(uint32_t));
		return *this;
	}

	raw_packet& raw_packet::operator >> (uint32_t& data)
	{
		if ((extractionOffset + sizeof(uint32_t)) > buf.size())
		{
			throw PacketExeption("[Packet::operator>>(uint32_t& data)] - Extraction exceeded buffer size!");
		}
		data = *reinterpret_cast<uint32_t*>(&buf[extractionOffset]);
		data = ntohl(data);
		extractionOffset += sizeof(uint32_t);

		return *this;
	}

	raw_packet& raw_packet::operator << (unsigned char data)
	{
		*this << (uint32_t)sizeof(data);
		Append(&data, sizeof(data));

		return *this;
	}

	raw_packet& raw_packet::operator >> (unsigned char& data)
	{
		uint32_t size = 0;
		*this >> size;

		if ((extractionOffset + size) > buf.size())
		{
			throw PacketExeption("[Packet::operator>>(unsigned char& data)] - Extraction exceeded buffer size!");
		}

		data = buf[extractionOffset];
		extractionOffset += size;

		return *this;
	}
}