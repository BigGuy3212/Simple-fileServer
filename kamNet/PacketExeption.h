#pragma once

#include<iostream>

namespace kamNet
{
	class PacketExeption
	{
	public:
		PacketExeption(std::string exeption)
			:exeption(exeption)
		{}

		const char* what()
		{
			return exeption.c_str();
		}

		std::string ToString()
		{
			return exeption;
		}

	private:
		std::string exeption;
	};
}