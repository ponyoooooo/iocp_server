#include "TextServer.h"
#include <iostream>
#include <format>
#include <chrono>

TextServer::TextServer() {}

TextServer::~TextServer() {}

void TextServer::OnConnect(const std::uint32_t clientIndex)
{
	std::cout << std::format("OnConnect Index={}", clientIndex) << '\n';

	PacketInfo packet{ clientIndex, PACKET_ID::SYS_USER_CONNECT, 0 };
	m_pPacketManager->PushSystemPacket(packet);
}

void TextServer::OnClose(const std::uint32_t clientIndex)
{
	std::cout << std::format("OnClose Index={}", clientIndex) << '\n';

	PacketInfo packet{ clientIndex, PACKET_ID::SYS_USER_DISCONNECT, 0 };
	m_pPacketManager->PushSystemPacket(packet);
}

void TextServer::OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData)
{
	std::cout << std::format("OnReceive Index={}, dataSize={}", clientIndex, size) << '\n';

	m_pPacketManager->ReceivePacketData(clientIndex, size, pData);
}

void TextServer::Run(const std::uint32_t maxClient)
{
	auto sendPacketFunc = [&](std::uint32_t clientIndex, std::uint16_t packetSize, char* pSendPacket)
		{
			SendMsg(clientIndex, packetSize, pSendPacket);
		};

	m_pPacketManager = std::make_unique<PacketManager>();
	m_pPacketManager->SendPacketFunc = sendPacketFunc;
	m_pPacketManager->Init(maxClient);
	m_pPacketManager->Run();

	StartServer(maxClient);
}

void TextServer::End()
{
	m_pPacketManager->End();

	DestryThread();
}