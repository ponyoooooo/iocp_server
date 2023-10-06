#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <memory>

// 클라이언트가 보낸 패킷을 저장하는 구조체
struct PacketData
{
	std::uint32_t m_sessionIndex = 0;

	[[nodiscard]] std::uint32_t GetSessionIndex() const
	{
		return m_sessionIndex;
	}

	void SetSessionIndex(const std::uint32_t sessionIndex)
	{
		m_sessionIndex = sessionIndex;
	}

	std::uint32_t m_dataSize = 0;

	[[nodiscard]] std::uint32_t GetDataSize() const
	{
		return m_dataSize;
	}

	void SetDataSize(const std::uint32_t dataSize)
	{
		m_dataSize = dataSize;
	}

	char* m_pPacketData = nullptr;

	[[nodiscard]] char* GetPacketData() const
	{
		return m_pPacketData;
	}

	void SetPacket(const PacketData& value)
	{
		m_sessionIndex = value.m_sessionIndex;
		m_dataSize = value.m_dataSize;

		m_pPacketData = new char[m_dataSize];
		std::memcpy(m_pPacketData, value.GetPacketData(), m_dataSize);
	}
	
	void SetPacketData(std::uint32_t sessionIndex_, std::uint32_t const dataSize_, const char* pPacketData_)
	{
		m_sessionIndex = sessionIndex_;
		m_dataSize = dataSize_;

		m_pPacketData = new char[m_dataSize];
		std::memcpy(m_pPacketData, pPacketData_, m_dataSize);
	}

	void Release()
	{
		delete[] m_pPacketData;
	}
};