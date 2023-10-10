// User.cpp

#include "User.h"

// �⺻ ������
User::User() = default;

// �Ҹ���
User::~User() = default;

// ����� �ʱ�ȭ �Լ�
void User::Init(const int index)
{
	m_index = index;
	// ��Ŷ ������ ���� �޸� �Ҵ�
	m_packetDataBuffer = std::make_unique<char[]>(PACKET_DATA_BUFFER_SIZE);
}

// ����� ���� �ʱ�ȭ �Լ�
void User::Clear()
{
	m_groundIndex = -1;
	m_userID.clear();
	m_isConfirm = false;
	m_curDomainState = DOMAIN_STATE::NONE;
	m_packetDataBufferWPos = 0;
	m_packetDataBufferRPos = 0;

	m_level = 1;
	m_exp = 0;
	m_maxExp = 100;
	m_hP = 50;
	m_maxHP = 50;
	m_mP = 50;
	m_maxMP = 50;
	m_attack = 10;
	m_defense = 5;
	m_gold = 100;
}

// �α��� ���� �Լ�
int User::SetLogin(const std::string& userID)
{
	m_curDomainState = DOMAIN_STATE::LOGIN;
	m_userID = userID;
	return 0; // ���������� �α��� ���� �Ϸ�
}

// ����� ���� �Լ�
void User::EnterGround(int groundNum)
{
	m_groundIndex = groundNum;
	m_curDomainState = DOMAIN_STATE::HUNTINGGROUND;
}

// ������ ���� ���� �Լ�
void User::SetDomainState(DOMAIN_STATE value)
{
	m_curDomainState = value;
}

// ���� ����� ��ȣ ��ȯ �Լ�
int User::GetCurrentGround() const
{
	return m_groundIndex;
}

// ��Ʈ��ũ ���� �ε��� ��ȯ �Լ�
int User::GetNetConnIdx() const
{
	return m_index;
}

// ����� ID ��ȯ �Լ�
std::string User::GetUserId() const
{
	return m_userID;
}

// ������ ���� ��ȯ �Լ�
User::DOMAIN_STATE User::GetDomainState() const
{
	return m_curDomainState;
}

// ��Ŷ ������ ���� �Լ�
void User::SetPacketData(const std::uint32_t dataSize, const char* pData)
{
	// ���� �����÷ο� ����
	if ((m_packetDataBufferWPos + dataSize) >= PACKET_DATA_BUFFER_SIZE)
	{
		auto remainDataSize = m_packetDataBufferWPos - m_packetDataBufferRPos;
		if (remainDataSize > 0)
		{
			std::copy(&m_packetDataBuffer[m_packetDataBufferRPos], &m_packetDataBuffer[m_packetDataBufferWPos], m_packetDataBuffer.get());
			m_packetDataBufferWPos = remainDataSize;
		}
		else
		{
			m_packetDataBufferWPos = 0;
		}
		m_packetDataBufferRPos = 0;
	}

	// ��Ŷ ������ ����
	std::copy(pData, pData + dataSize, &m_packetDataBuffer[m_packetDataBufferWPos]);
	m_packetDataBufferWPos += dataSize;
}

// ��Ŷ ���� ��ȯ �Լ�
PacketInfo User::GetPacket()
{
	const int PACKET_SIZE_LENGTH = 2;
	const int PACKET_TYPE_LENGTH = 2;
	short packetSize = 0;

	std::uint32_t remainByte = m_packetDataBufferWPos - m_packetDataBufferRPos;

	if (remainByte < PACKET_HEADER_LENGTH)
	{
		return PacketInfo();
	}

	auto pHeader = reinterpret_cast<PACKET_HEADER*>(&m_packetDataBuffer[m_packetDataBufferRPos]);

	if (pHeader->PacketLength > remainByte)
	{
		return PacketInfo();
	}

	PacketInfo packetInfo;
	packetInfo.PacketId = pHeader->PacketId;
	packetInfo.DataSize = pHeader->PacketLength;
	packetInfo.pDataPtr = &m_packetDataBuffer[m_packetDataBufferRPos];

	m_packetDataBufferRPos += pHeader->PacketLength;

	return packetInfo;
}
