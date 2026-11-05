#pragma once
#include<stdint.h>

namespace kamNet
{
	enum packetType : uint16_t
	{
		PT_Invalid,
		PT_info,
		PT_file,
		PT_upload,
		PT_download,
		PT_fileEnd,
		PT_serverInfo
	};
}