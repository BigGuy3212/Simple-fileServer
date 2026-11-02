#pragma once
#include<stdint.h>

namespace kamNet
{
	enum packetType : uint16_t
	{
		PT_Invalid,
		PT_ChatMessage,
		PT_IntegerArray,
		PT_file,
		PT_Request,
		PT_Save
	};
}