#include"Client.h"
#include<iostream>

using namespace kamNet;

bool Client::connect(IPEndpoint ip)
{
	isConnected = false;
	if (Network::init())
	{
		std::cout << "Winsock API started up successfully!\n";

		socket = Socket(ip.GetIpVersion());
		if (socket.CreateSocket() == KResult::K_Sucess)
		{
			if (socket.SetBlocking(true) != KResult::K_Sucess)
				return false;

			std::cout << "Socket created sucessfully!\n";

			if (socket.Connect(ip) == KResult::K_Sucess)
			{
				std::cout << "Succesfully connected to the server on " << ip.GetIpString() << "\n";
				isConnected = true;
				return true;
			}
			else
			{
				std::cerr << "Failed to connect to the server!\n";
			}
			socket.CloseSocket();
		}
		else
		{
			std::cerr << "Failed to create socket!\n";
		}
	}
	return false;
}

bool Client::IsConnected()
{
	return isConnected;
}

bool Client::Frame()
{
	/*
	KResult result;
	
	Packet StringPacket(packetType::PT_ChatMessage);
	StringPacket << std::string("The string packet!");

	Packet integerPacket(packetType::PT_IntegerArray);
	uint32_t arraySize = 6;
	uint32_t integerArray[6] = { 2, 6, 3, 2, 43, 2 };
	integerPacket << arraySize;
	for (auto integer : integerArray)
	{
		integerPacket << integer;
	}

	std::cout << "sent packet" << std::endl;

	if (rand() % 2 == 0)
	{
		result = socket.Send(StringPacket);
	}
	else
	{
		result = socket.Send(integerPacket);
	}

	if (result != KResult::K_Sucess)
	{
		isConnected = false;
		return false;
	}

	Sleep(500);
	return true;*/

	Packet incomingPacket;
	if (socket.Recv(incomingPacket) != KResult::K_Sucess)
	{
		std::cout << "packet lost" << std::endl;
		isConnected = false;
		return false;
	}

	if (!processPacket(incomingPacket))
	{
		isConnected = false;
		return false;
	}

	return true;
}

bool Client::processPacket(Packet& packet)
{
	switch (packet.GetPacketType())
	{
	case packetType::PT_ChatMessage:
	{
		std::string chatmessage;
		packet >> chatmessage;
		std::cout << "Chat Message: " << chatmessage << std::endl;
		break;
	}

	case packetType::PT_IntegerArray:
	{
		uint32_t arraySize = 0;
		packet >> arraySize;
		std::cout << "Array Size: " << arraySize << std::endl;
		for (uint32_t i = 0; i < arraySize; i++)
		{
			uint32_t element = 0;
			packet >> element;
			std::cout << "Element[" << i << "] - " << element << std::endl;
		}
		break;
	}
	/*case packetType::PT_Save:
	{
		std::string request;
		packet >> request;

		newest_file = "C:/Users/kamil/Downloads/" + request;

		std::ofstream newfile("C:/Users/kamil/Downloads/" + request, std::ios::binary);

		file.open(newest_file, std::ios::out | std::ios::binary);

		break;
	}

	case packetType::PT_file:
	{
		packet >> buf;

		std::cout << buf << std::endl;

		write_to_file(buf);

		buf.clear();

		break;
	}*/

	default:
		return false;
	}
	return true;
}