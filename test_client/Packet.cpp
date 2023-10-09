#include "Packet.h"

RawPacketData::RawPacketData() : m_clientIndex(0), m_dataSize(0), m_pPacketData(nullptr) {}

RawPacketData::~RawPacketData()
{
    Release();
}

// 다른 RawPacketData 객체로부터 패킷 데이터를 설정하는 함수
void RawPacketData::Set(const RawPacketData& value)
{
    // 기존에 할당된 메모리가 있다면 해제
    Release();

    m_clientIndex = value.m_clientIndex;
    m_dataSize = value.m_dataSize;

    m_pPacketData = std::make_unique<char[]>(m_dataSize);
    std::copy(value.m_pPacketData.get(), value.m_pPacketData.get() + m_dataSize, m_pPacketData.get());
}

// 개별 값으로 패킷 데이터를 설정하는 함수
void RawPacketData::Set(std::uint32_t clientIndex, std::uint32_t dataSize, const char* pData)
{
    // 기존에 할당된 메모리가 있다면 해제
    Release();

    m_clientIndex = clientIndex;
    m_dataSize = dataSize;

    m_pPacketData = std::make_unique<char[]>(m_dataSize);
    std::copy(pData, pData + m_dataSize, m_pPacketData.get());
}

// 동적으로 할당된 패킷 데이터 메모리를 해제하는 함수
void RawPacketData::Release()
{
    m_pPacketData.reset();
    m_dataSize = 0;
}
