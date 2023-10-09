#include "PacketData.h"
#include <cstring>

std::uint32_t PacketData::GetSessionIndex() const
{
    return m_sessionIndex;
}

void PacketData::SetSessionIndex(const std::uint32_t sessionIndex)
{
    m_sessionIndex = sessionIndex;
}

std::uint32_t PacketData::GetDataSize() const
{
    return m_dataSize;
}

void PacketData::SetDataSize(const std::uint32_t dataSize)
{
    m_dataSize = dataSize;
}

char* PacketData::GetPacketData() const
{
    return m_pPacketData;
}

void PacketData::SetPacket(const PacketData& value)
{
    m_sessionIndex = value.m_sessionIndex;
    m_dataSize = value.m_dataSize;

    m_pPacketData = new char[m_dataSize];
    std::memcpy(m_pPacketData, value.GetPacketData(), m_dataSize);
}

void PacketData::SetPacketData(std::uint32_t sessionIndex, std::uint32_t const dataSize, const char* pPacketData)
{
    m_sessionIndex = sessionIndex;
    m_dataSize = dataSize;

    m_pPacketData = new char[m_dataSize];
    std::memcpy(m_pPacketData, pPacketData, m_dataSize);
}

void PacketData::Release()
{
    delete[] m_pPacketData;
}
