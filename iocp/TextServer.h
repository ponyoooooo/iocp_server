#pragma once
#include "IOCPServer.h"
#include "PacketManager.h"
#include "Packet.h"
#include <thread>
#include <mutex>
#include <deque>

class TextServer : public IOCPServer
{
public:
	TextServer();
	virtual ~TextServer();

	virtual void OnConnect(const std::uint32_t clientIndex) override;
	virtual void OnClose(const std::uint32_t clientIndex) override;
	virtual void OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData) override;

	void Run(const std::uint32_t maxClient);
	void End();

private:
	std::unique_ptr<PacketManager> m_pPacketManager;
};
