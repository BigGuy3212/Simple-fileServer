#include"Client.h"
#include<iostream>
#include<sstream>
#include<string>

#undef max
#undef min

using namespace kamNet;

int packetsRecived = 0;
int packetsSent = 0;

Console console;

#pragma region Network

#pragma region NetworkInit
bool Client::connect(IPEndpoint ip)
{
	isConnected = false;

	socket = Socket(ip.GetIpVersion());
	if (socket.CreateSocket() == KResult::K_Sucess)
	{
		if (socket.SetBlocking(true) != KResult::K_Sucess)
			return false;

		if (socket.Connect(ip) == KResult::K_Sucess)
		{
			if (socket.SetBlocking(false) == KResult::K_Sucess)
			{
				connection = TCPconnection(socket, ip);
				masterFD.fd = connection.socket.GetHandle();
				masterFD.events = POLLRDNORM;
				masterFD.revents = 0;


				std::shared_ptr<Packet> helloPacket = std::make_shared<Packet>(packetType::PT_info);
				*helloPacket << std::string("Hello from client\n");

				connection.pm_outgoing.Append(helloPacket);

				isConnected = true;
				return true;
			}
		}
		socket.CloseSocket();
	}

	return false;
}
#pragma endregion NetworkInit

bool Client::IsConnected()
{
	return isConnected;
}

#pragma region NetworkFrame

bool Client::Frame()
{
	if (connection.pm_outgoing.HasPendingPackets())
	{
		masterFD.events = POLLRDNORM | POLLWRNORM;
	}

	useFD = masterFD;

	if (WSAPoll(&useFD, 1, 1) > 0)
	{

		if (useFD.revents & POLLERR)
		{
			CloseConnection("Poll error");

			return false;
		}

		if (useFD.revents & POLLHUP)
		{
			CloseConnection("Poll hangup");

			return false;
		}

		if (useFD.revents & POLLNVAL)
		{
			CloseConnection("Invalid socket");

			return false;
		}

		if (useFD.revents & POLLRDNORM)
		{

			int bytesRecived = 0;

			if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
			{
				bytesRecived = recv(useFD.fd, (char*)&connection.pm_incoming.currentPacketSize + connection.pm_incoming.currentPacketExtractionOffset, sizeof(uint16_t) - connection.pm_incoming.currentPacketExtractionOffset, 0);
			}
			else
			{
				bytesRecived = recv(useFD.fd, (char*)&connection.buffer + connection.pm_incoming.currentPacketExtractionOffset, connection.pm_incoming.currentPacketSize - connection.pm_incoming.currentPacketExtractionOffset, 0);
			}

			if (bytesRecived == 0)
			{
				CloseConnection("Recv == 0");

				return false;
			}

			if (bytesRecived == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				if (error != WSAEWOULDBLOCK)
				{
					CloseConnection("Recv < 0");
					return false;
				}

			}

			if (bytesRecived > 0)
			{
				connection.pm_incoming.currentPacketExtractionOffset += bytesRecived;
				if (connection.pm_incoming.currentTask == PacketManagerTask::ProcessPacketSize)
				{
					if (connection.pm_incoming.currentPacketExtractionOffset == sizeof(uint16_t))
					{
						connection.pm_incoming.currentPacketSize = ntohs(connection.pm_incoming.currentPacketSize);
						if (connection.pm_incoming.currentPacketSize > kamNet::MaxPacketSize)
						{
							CloseConnection("Packet too large!");
							return false;
						}
						connection.pm_incoming.currentPacketExtractionOffset = 0;
						connection.pm_incoming.currentTask = PacketManagerTask::ProcessPacketContents;
					}
				}
				else
				{
					if (connection.pm_incoming.currentPacketExtractionOffset == connection.pm_incoming.currentPacketSize)
					{
						std::shared_ptr<Packet> packet = std::make_shared<Packet>();

						packet->buf.resize(connection.pm_incoming.currentPacketSize);
						memcpy(&packet->buf[0], &connection.buffer, connection.pm_incoming.currentPacketSize);

						connection.pm_incoming.Append(packet);

						connection.pm_incoming.currentPacketSize = 0;
						connection.pm_incoming.currentPacketExtractionOffset = 0;
						connection.pm_incoming.currentTask = PacketManagerTask::ProcessPacketSize;
					}
				}
			}
		}

		if (useFD.revents & POLLWRNORM)
		{
			PacketManager& pm = connection.pm_outgoing;

			while (pm.HasPendingPackets())
			{
				if (pm.currentTask == PacketManagerTask::ProcessPacketSize)
				{
					pm.currentPacketSize = pm.Retrive()->buf.size();
					uint16_t bigEndiaPacketSize = htons(pm.currentPacketSize);
					int bytesSent = send(useFD.fd, (char*)(&bigEndiaPacketSize) + pm.currentPacketExtractionOffset, sizeof(uint16_t) - pm.currentPacketExtractionOffset, 0);
					if (bytesSent > 0)
					{
						pm.currentPacketExtractionOffset += bytesSent;
					}

					if (pm.currentPacketExtractionOffset == sizeof(uint16_t))
					{
						pm.currentPacketExtractionOffset = 0;
						pm.currentTask = PacketManagerTask::ProcessPacketContents;
					}
					else
					{
						break;
					}
				}
				else
				{
					char* pBuffer = &pm.Retrive()->buf[0];
					
					int bytesSent = send(useFD.fd, (char*)(pBuffer)+pm.currentPacketExtractionOffset, pm.currentPacketSize - pm.currentPacketExtractionOffset, 0);
					if (bytesSent > 0)
					{
						pm.currentPacketExtractionOffset += bytesSent;
					}

					if (pm.currentPacketExtractionOffset == pm.currentPacketSize)
					{
						pm.currentPacketExtractionOffset = 0;
						pm.currentTask = PacketManagerTask::ProcessPacketSize;
						pm.pop();
					}
					else
					{
						break;
					}
				}
			}
			if (connection.pm_outgoing.HasPendingPackets())
			{
				masterFD.events = POLLRDNORM;
			}
		}
	}
	while (connection.pm_incoming.HasPendingPackets())
	{
		std::shared_ptr<Packet> frontPacket = connection.pm_incoming.Retrive();
		if (!processPacket(frontPacket))
		{
			CloseConnection("Failed to process incoming packet!\n");
			return false;
		}
		connection.pm_incoming.pop();
	}
}

#pragma endregion NetworkFrame

#pragma region PacketHandling

bool Client::processPacket(std::shared_ptr<Packet> packet)
{
	switch (packet->GetPacketType())
	{
	case packetType::PT_info:
	{
		std::string chatmessage;
		*packet >> chatmessage;

		console.log(chatmessage);

		break;
	}
	case packetType::PT_upload:
	{
		std::string request;
		*packet >> request;

		file = std::ofstream(latestFile.string() + request, std::ios::binary);

		break;
	}

	case packetType::PT_file:
	{
		*packet >> buf;

		writeToFile(buf);

		packetsRecived++;

		break;
	}

	case packetType::PT_fileEnd:
	{
		file.close();

		console.log("Recived " + std::to_string(packetsRecived) + " packets");

		packetsRecived = 0;
		
		break;
	}

	case packetType::PT_serverInfo:
	{
		std::string info;

		*packet >> info;

		std::stringstream stringStream(info);

		std::string temp;

		std::vector<std::string> parsedInfo;

		while (std::getline(stringStream, temp, ','))
		{
			parsedInfo.push_back(temp);
		}

		document->GetElementById("servername")->SetInnerRML(parsedInfo[2]);
		document->GetElementById("ip")->SetInnerRML("IP: " + parsedInfo[0]);
		document->GetElementById("port")->SetInnerRML("Port: " + parsedInfo[1]);
		
		break;
	}

	default:
		console.log("Unknown packet type - " + std::to_string(packet->GetPacketType()));
		return false;
	}
	return true;
}

#pragma endregion PacketHandling

void Client::CloseConnection(std::string reason)
{
	masterFD.fd = 0;
	isConnected = false;
	connection.Close();

	document->GetElementById("servername")->SetInnerRML("Not connected");
	document->GetElementById("ip")->SetInnerRML("IP: ");
	document->GetElementById("port")->SetInnerRML("Port: ");
}

#pragma region FileHandling
void Client::sendFile(std::filesystem::path path)
{
	console.log("sending file...");

	std::shared_ptr<Packet> request = std::make_shared<Packet>(packetType::PT_upload);
	*request << std::string(path.filename().string());

	connection.pm_outgoing.Append(request);

	int bytesRead = 0;

	std::ifstream file(path.string(), std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		console.log("could not open file!");
	}

	std::streamsize size = file.tellg();

	std::vector<std::byte> bytes(size);

	file.seekg(0, std::ios::beg);

	file.read(reinterpret_cast<char*>(bytes.data()), size);

	const char* buf = reinterpret_cast<const char*>(bytes.data());
	unsigned char packet_buf[40];
	std::string str_buf;

	for(int i = 0;size >= bytesRead; i++)
	{
		std::shared_ptr<Packet> packet = std::make_shared<Packet>(packetType::PT_file);

		for (int j = 0; j < 40; j++)
		{
			packet_buf[j] = buf[bytesRead];

			bytesRead++;
		}

		str_buf.assign(reinterpret_cast<char*>(packet_buf), sizeof(str_buf));
		*packet << str_buf;

		Sleep(1);
		
		connection.pm_outgoing.Append(packet);

		packetsSent++;
	}

	file.close();

	str_buf.clear();
	bytes.clear();

	std::shared_ptr<Packet> endFile = std::make_shared<Packet>(packetType::PT_fileEnd);
	*endFile << std::string("end of file");

	connection.pm_outgoing.Append(endFile);

	console.log("Sent " + std::to_string(packetsSent) + " packets");

	packetsSent = 0;
}

void Client::writeToFile(std::string& data)
{
	if (file.is_open())
	{
		file.write(data.data(), data.size());
	}
}

void Client::downloadFile(std::string path)
{
	std::shared_ptr<Packet> download = std::make_shared<Packet>(packetType::PT_download);
	*download << std::string(path);

	connection.pm_outgoing.Append(download);
}
#pragma endregion FileHandling

#pragma endregion Network

bool Client::WindowInit()
{
	runing = true;

	Shell::Initialize();

	Backend::Initialize("Simple-FileServer Client", 1200, 960, true);

	Rml::SetSystemInterface(Backend::GetSystemInterface());
	Rml::SetRenderInterface(Backend::GetRenderInterface());

	if (!Rml::Initialise())
	{
		return false;
	}

	context = Rml::CreateContext("main", Rml::Vector2i(1200, 960));

	Rml::Debugger::Initialise(context);

	Rml::LoadFontFace("UI/Oswald-Regular.ttf");

	Rml::DataModelConstructor constructor = context->CreateDataModel("IP");

	constructor.Bind("ip", &ip);
	constructor.Bind("port", &portString);

	document = context->LoadDocument("UI/document.rml");
	if (!document)
	{
		Rml::Shutdown();
		return false;
	}

	document->Show();

	console = Console(document->GetElementById("terminal"), document);
	console.idenifier = ">>> ";
	
	this->LoadEventListeners();

	return true;
}

void Client::WindowFrame()
{
	runing = Backend::ProcessEvents(context, &Shell::ProcessKeyDownShortcuts, true);

	context->Update();

	Backend::BeginFrame();
	context->Render();
	Backend::PresentFrame();
	
}

bool Client::IsRunning()
{
	return runing;
}

void Client::shutdown()
{
	Rml::Shutdown();
	Backend::Shutdown();
}

EventListener::EventListener(std::function<void()> func)
{
	function = func;
}

void Client::LoadEventListeners()
{
	//Send button
	Rml::Element* button = document->GetElementById("sendbutton");

	EventListener* listener = new EventListener([this]() {
		sendFile(filepath.string());
		});

	button->AddEventListener(Rml::EventId::Click, listener);

	//Open file explorer
	Rml::Element* openFile = document->GetElementById("openFileDialog");

	EventListener* openFilelistener = new EventListener([this]() {
		filepath = this->OpenFileExplorer();

		Rml::Element* path = document->GetElementById("uploadPath");
		path->SetInnerRML(filepath.filename().string());
		});

	openFile->AddEventListener(Rml::EventId::Click, openFilelistener);

	//Connect button
	Rml::Element* connectButton = document->GetElementById("connect");

	EventListener* connectButtonListener = new EventListener([this]() {
		port = stous(portString);
		connect(IPEndpoint(ip.data(), port));
		});

	connectButton->AddEventListener(Rml::EventId::Click, connectButtonListener);

	//disconnect button
	Rml::Element* disconnectButton = document->GetElementById("disconnect");

	EventListener* disconnectButtonListener = new EventListener([this]() {
		CloseConnection("Disconnect");
		});

	disconnectButton->AddEventListener(Rml::EventId::Click, disconnectButtonListener);

	//reconnect button
	Rml::Element* reconnectButton = document->GetElementById("reconnect");

	EventListener* reconnectButtonListener = new EventListener([this]() {
		port = stous(portString);

		if (!IsConnected())
		{
			connect(IPEndpoint(ip.data(), port));
		}
		});

	reconnectButton->AddEventListener(Rml::EventId::Click, reconnectButtonListener);

	//Clear console
	Rml::Element* clearConsole = document->GetElementById("clearConsole");

	EventListener* clearConsoleListener = new EventListener([this]() {
		console.clear();
		});

	clearConsole->AddEventListener(Rml::EventId::Click, clearConsoleListener);

	//Download
	Rml::Element* download = document->GetElementById("downloadButton");

	EventListener* downloadListener = new EventListener([this]() {
		downloadFile("");
		});

	//DownloadPath
	Rml::Element* downloadPath = document->GetElementById("openDownloadFileDialog");

	EventListener* downloadPathListener = new EventListener([this]() {
		latestFile = OpenFileExplorer();
		});

	downloadPath->AddEventListener(Rml::EventId::Click, downloadPathListener);
}

std::filesystem::path Client::OpenFileExplorer()
{
	std::wstring path = L"";

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileOpenDialog* pFileOpen;

		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
		if (SUCCEEDED(hr)) {
			hr = pFileOpen->Show(NULL);

			if (SUCCEEDED(hr)) {
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					if (SUCCEEDED(hr)) {
						path = pszFilePath;
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}

	if (path != L"")
	{
		return std::filesystem::path(path);
	}
	else
	{
		return std::filesystem::path("");
	}
}

unsigned short Client::stous(std::string string)
{
	int number = std::stoi(string);

	if (number < std::numeric_limits<short>::min() || number > std::numeric_limits<short>::max())
	{
		throw std::out_of_range("Value out of range for SHORT!");
	}

	if (number < 0)
	{
		throw std::out_of_range("Value too small for unsigned short!");
	}

	return static_cast<unsigned short>(number);
}

#pragma region Console

Console::Console(Rml::Element* console, Rml::ElementDocument* document)
{
	this->console = console;
	this->document = document;
}

void Console::log(std::string line)
{
	if (amountOfLogs > 11)
	{
		console->RemoveChild(document->GetElementById("console"));
		console->RemoveChild(document->GetElementById("nextLine"));
		amountOfLogs--;
	}

	Rml::ElementPtr textObj = document->CreateElement("p");
	Rml::ElementPtr text = document->CreateTextNode(idenifier + line);
	textObj->AppendChild(std::move(text));
	textObj->SetId("console");

	console->AppendChild(std::move(textObj));
	Rml::ElementPtr nextline = document->CreateElement("br");
	nextline->SetId("nextLine");
	console->AppendChild(std::move(nextline));

	consoleLine.push_back(line);
	amountOfLogs++;
}

void Console::clear()
{
	for (int i = 0; i < amountOfLogs; i++)
	{		
		console->RemoveChild(document->GetElementById("console"));
	}
}
#pragma endregion Console