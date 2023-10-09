#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <memory>
#include "Define.h"

#pragma pack(push,1)

// 패킷 헤더 구조체
struct PACKET_HEADER
{
	std::uint16_t PacketLength;
	PACKET_ID PacketId;
	std::uint8_t Type;
};

// 원시 패킷 데이터 구조체
struct RawPacketData
{
	std::uint32_t m_clientIndex = 0;
	uint32_t m_dataSize = 0;
	std::unique_ptr<char[]> m_pPacketData;

	RawPacketData();
	~RawPacketData();
	void Set(const RawPacketData& value);
	void Set(std::uint32_t clientIndex, std::uint32_t dataSize, const char* pData);
	void Release();
};

// 패킷 정보 구조체
struct PacketInfo
{
	uint32_t ClientIndex = 0;
	PACKET_ID PacketId = PACKET_ID::NONE;
	uint16_t DataSize = 0;
	char* pDataPtr = nullptr;
};

inline const std::uint32_t PACKET_HEADER_LENGTH = sizeof(PACKET_HEADER);

// 로그인 요청 패킷 구조체
struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1];
	char UserPW[MAX_USER_PW_LEN + 1];
};

inline const std::uint32_t LOGIN_REQUEST_PACKET_SZIE = sizeof(LOGIN_REQUEST_PACKET);

inline const int MAX_INVENTORY_ITEMS = 10; // 인벤토리 아이템 최대 개수 (예시로 10개로 설정)

struct Item {
	uint32_t ItemID;     // 아이템 고유 ID
	uint16_t Quantity;   // 아이템 개수
};

// 로그인 응답 패킷 구조체
struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	uint16_t Result;     // 로그인 결과

	uint16_t Level = 1;      // 캐릭터 레벨
	uint32_t Exp = 0;        // 경험치
	uint32_t MaxExp = 100;     // 다음 레벨로 넘어가기 위한 필요 경험치
	uint32_t HP = 50;         // 체력
	uint32_t MaxHP = 50;      // 최대 체력
	uint32_t MP = 50;         // 마력
	uint32_t MaxMP = 50;      // 최대 마력
	uint32_t Attack = 10;     // 공격력
	uint32_t Defense = 5;    // 방어력
	uint32_t Gold = 100;       // 돈

	//Item Inventory[MAX_INVENTORY_ITEMS]; // 인벤토리 아이템 리스트
};

// 사냥터 입장 요청 패킷 구조체
struct GROUND_ENTER_REQUEST_PACKET : public PACKET_HEADER
{
	int32_t GroundNumber;
};

// 사냥터 입장 응답 패킷 구조체
struct GROUND_ENTER_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};

// 사냥터 나가기 요청 패킷 구조체
struct GROUND_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{
};

// 사냥터 나가기 응답 패킷 구조체
struct GROUND_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};

// 사냥터 요청 패킷 구조체
struct GROUND_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	char Message[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};

// 사냥터 응답 패킷 구조체
struct GROUND_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};

// 사냥터 알림 패킷 구조체
struct GROUND_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1] = { 0, };
	char Msg[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};

// 사냥터 유저 입장 알림 패킷 구조체
struct GROUND_USER_ENTER_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserId[50];
};

// 사냥터 유저 퇴장 알림 패킷 구조체
struct GROUND_USER_LEAVE_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserId[50];
};

#pragma pack(pop)
