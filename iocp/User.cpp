// User.cpp

#include "User.h"

// 기본 생성자
User::User() = default;

// 소멸자
User::~User() = default;

// 사용자 초기화 함수
void User::Init(const int index)
{
	m_index = index;
	// 패킷 데이터 버퍼 메모리 할당
	m_packetDataBuffer = std::make_unique<char[]>(PACKET_DATA_BUFFER_SIZE);
}

// 사용자 정보 초기화 함수
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

// 로그인 설정 함수
int User::SetLogin(const std::string& userID)
{
	m_curDomainState = DOMAIN_STATE::LOGIN;
	m_userID = userID;
	return 0; // 성공적으로 로그인 설정 완료
}

// 사냥터 입장 함수
void User::EnterGround(int groundNum)
{
	m_groundIndex = groundNum;
	m_curDomainState = DOMAIN_STATE::HUNTINGGROUND;
}

// 도메인 상태 설정 함수
void User::SetDomainState(DOMAIN_STATE value)
{
	m_curDomainState = value;
}

// 현재 사냥터 번호 반환 함수
int User::GetCurrentGround() const
{
	return m_groundIndex;
}

// 네트워크 연결 인덱스 반환 함수
int User::GetNetConnIdx() const
{
	return m_index;
}

// 사용자 ID 반환 함수
std::string User::GetUserId() const
{
	return m_userID;
}

// 도메인 상태 반환 함수
User::DOMAIN_STATE User::GetDomainState() const
{
	return m_curDomainState;
}

// 패킷 데이터 설정 함수
void User::SetPacketData(const std::uint32_t dataSize, const char* pData)
{
	// 버퍼 오버플로우 방지
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

	// 패킷 데이터 복사
	std::copy(pData, pData + dataSize, &m_packetDataBuffer[m_packetDataBufferWPos]);
	m_packetDataBufferWPos += dataSize;
}

// 패킷 정보 반환 함수
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
