#include "Packet.h"

RawPacketData::RawPacketData() : m_clientIndex(0), m_dataSize(0), m_pPacketData(nullptr) {}

RawPacketData::~RawPacketData()
{
    Release();
}

// �ٸ� RawPacketData ��ü�κ��� ��Ŷ �����͸� �����ϴ� �Լ�
void RawPacketData::Set(const RawPacketData& value)
{
    // ������ �Ҵ�� �޸𸮰� �ִٸ� ����
    Release();

    m_clientIndex = value.m_clientIndex;
    m_dataSize = value.m_dataSize;

    m_pPacketData = std::make_unique<char[]>(m_dataSize);
    std::copy(value.m_pPacketData.get(), value.m_pPacketData.get() + m_dataSize, m_pPacketData.get());
}

// ���� ������ ��Ŷ �����͸� �����ϴ� �Լ�
void RawPacketData::Set(std::uint32_t clientIndex, std::uint32_t dataSize, const char* pData)
{
    // ������ �Ҵ�� �޸𸮰� �ִٸ� ����
    Release();

    m_clientIndex = clientIndex;
    m_dataSize = dataSize;

    m_pPacketData = std::make_unique<char[]>(m_dataSize);
    std::copy(pData, pData + m_dataSize, m_pPacketData.get());
}

// �������� �Ҵ�� ��Ŷ ������ �޸𸮸� �����ϴ� �Լ�
void RawPacketData::Release()
{
    m_pPacketData.reset();
    m_dataSize = 0;
}
