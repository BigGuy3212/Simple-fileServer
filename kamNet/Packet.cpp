#include "packet.h"
#include "constants.h"

namespace kamNet
{
	void Packet::Clear()
	{
		buf.resize(sizeof(packetType));
		AssignPacketType(packetType::PT_Invalid);
		extractionoffset = sizeof(packetType);
	}

	void Packet::Append(const void* data, uint32_t size)
	{
		if ((buf.size() + size) > kamNet::MaxPacketSize)
		{
			throw PacketExeption("[Packet::Append(const void* data, uint32_t size)] - Packet exeded max packet size!");
		}
		buf.insert(buf.end(), (char*)data, (char*)data + size);
	}

	Packet& Packet::operator<<(uint32_t data)
	{
		data = htonl(data);
		Append(&data, sizeof(uint32_t));
		return *this;
	}

	Packet& Packet::operator>>(uint32_t& data) //extraction
	{
		if ((extractionoffset + sizeof(uint32_t)) > buf.size())
		{
			throw PacketExeption("[Packet::operator>>(uint32_t& data)] - Extraction exceeded buffer size!");
		}
		data = *reinterpret_cast<uint32_t*>(&buf[extractionoffset]);
		data = ntohl(data);
		extractionoffset += sizeof(uint32_t);

		return *this;
	}

	Packet& Packet::operator << (const std::string& data)
	{
		*this << (uint32_t)data.size();
		Append(data.data(), data.size());

		return *this;
	}

	Packet& Packet::operator >> (std::string& data) //extraction
	{
		data.clear();

		uint32_t stringSize = 0;
		*this >> stringSize;

		if ((extractionoffset + stringSize) > buf.size())
		{
			throw PacketExeption("[Packet::operator>>(std::string& data)] - Extraction exceeded buffer size!");
		}
		
		data.resize(stringSize);
		data.assign(&buf[extractionoffset], stringSize);
		extractionoffset += stringSize;

		return *this;
	}

	packetType Packet::GetPacketType()
	{
		packetType* packettypePtr = reinterpret_cast<packetType*>(&buf[0]);
		return static_cast<packetType>(ntohs(*packettypePtr));
	}

	void Packet::AssignPacketType(packetType packettype)
	{
		packetType* packettypePtr = reinterpret_cast<packetType*>(&buf[0]);
		*packettypePtr = static_cast<packetType>(htons(packettype));
	}
	
	Packet::Packet(packetType packettype)
	{
		Clear();
		AssignPacketType(packettype);
	}
}