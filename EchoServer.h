#pragma once
#include "IOCPServer.h"
#include "PacketData.h"

class EchoServer : public IOCPServer
{
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;

	virtual void OnConnect(const std::uint32_t clientIndex_) override
	{
		std::cout << std::format("OnConnect Index={}", clientIndex_) << '\n';
	}

	virtual void OnClose(const std::uint32_t clientIndex_) override
	{
		std::cout << std::format("OnClose Index={}", clientIndex_) << '\n';
	}

	virtual void OnReceive(const std::uint32_t clientIndex_, const std::uint32_t size_, char* pData_) override
	{
		std::cout << std::format("OnReceive Index={}, dataSize={}", clientIndex_, size_) << '\n';

		PacketData packet;
		packet.SetPacketData(clientIndex_, size_, pData_);

		std::lock_guard<std::mutex> guard(m_lock);
		m_packetDataQueue.push_back(packet);
	}

	void Run(const std::uint32_t maxClient)
	{
		m_isRunProcessThread = true;
		m_processThread = std::thread([this]() { ProcessPacket(); });

		StartServer(maxClient);
	}

	void End()
	{
		m_isRunProcessThread = false;

		if (m_processThread.joinable())
		{
			m_processThread.join();
		}

		DestryThread();
	}

private:
	void ProcessPacket()
	{
		while (m_isRunProcessThread)
		{
			auto packetData = DequePacketData();
			if (packetData.GetDataSize() != 0)
			{
				SendMsg(packetData.GetSessionIndex(), packetData.GetDataSize(), packetData.GetPacketData());
				PacketFrontPop();
				continue;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	PacketData DequePacketData()
	{
		PacketData packetData;

		std::lock_guard<std::mutex> guard(m_lock);
		if (m_packetDataQueue.empty())
		{
			return PacketData();
		}

		return m_packetDataQueue.front();
	}

	void PacketFrontPop()
	{
		std::lock_guard<std::mutex> guard(m_lock);

		m_packetDataQueue.front().Release();
		m_packetDataQueue.pop_front();
	}

	bool m_isRunProcessThread = false;
	std::thread m_processThread;

	std::mutex m_lock;
	std::deque<PacketData> m_packetDataQueue;
};
