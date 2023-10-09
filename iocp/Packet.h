#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <memory>
#include "Define.h"

#pragma pack(push,1)

// ��Ŷ ��� ����ü
struct PACKET_HEADER
{
	std::uint16_t PacketLength;
	PACKET_ID PacketId;
	std::uint8_t Type;
};

// ���� ��Ŷ ������ ����ü
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

// ��Ŷ ���� ����ü
struct PacketInfo
{
	uint32_t ClientIndex = 0;
	PACKET_ID PacketId = PACKET_ID::NONE;
	uint16_t DataSize = 0;
	char* pDataPtr = nullptr;
};

inline const std::uint32_t PACKET_HEADER_LENGTH = sizeof(PACKET_HEADER);

// �α��� ��û ��Ŷ ����ü
struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1];
	char UserPW[MAX_USER_PW_LEN + 1];
};

inline const std::uint32_t LOGIN_REQUEST_PACKET_SZIE = sizeof(LOGIN_REQUEST_PACKET);

inline const int MAX_INVENTORY_ITEMS = 10; // �κ��丮 ������ �ִ� ���� (���÷� 10���� ����)

struct Item {
	uint32_t ItemID;     // ������ ���� ID
	uint16_t Quantity;   // ������ ����
};

// �α��� ���� ��Ŷ ����ü
struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	uint16_t Result;     // �α��� ���

	uint16_t Level = 1;      // ĳ���� ����
	uint32_t Exp = 0;        // ����ġ
	uint32_t MaxExp = 100;     // ���� ������ �Ѿ�� ���� �ʿ� ����ġ
	uint32_t HP = 50;         // ü��
	uint32_t MaxHP = 50;      // �ִ� ü��
	uint32_t MP = 50;         // ����
	uint32_t MaxMP = 50;      // �ִ� ����
	uint32_t Attack = 10;     // ���ݷ�
	uint32_t Defense = 5;    // ����
	uint32_t Gold = 100;       // ��

	//Item Inventory[MAX_INVENTORY_ITEMS]; // �κ��丮 ������ ����Ʈ
};

// ����� ���� ��û ��Ŷ ����ü
struct GROUND_ENTER_REQUEST_PACKET : public PACKET_HEADER
{
	int32_t GroundNumber;
};

// ����� ���� ���� ��Ŷ ����ü
struct GROUND_ENTER_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};

// ����� ������ ��û ��Ŷ ����ü
struct GROUND_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{
};

// ����� ������ ���� ��Ŷ ����ü
struct GROUND_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};

// ����� ��û ��Ŷ ����ü
struct GROUND_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	char Message[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};

// ����� ���� ��Ŷ ����ü
struct GROUND_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t Result;
};

// ����� �˸� ��Ŷ ����ü
struct GROUND_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1] = { 0, };
	char Msg[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};

// ����� ���� ���� �˸� ��Ŷ ����ü
struct GROUND_USER_ENTER_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserId[50];
};

// ����� ���� ���� �˸� ��Ŷ ����ü
struct GROUND_USER_LEAVE_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserId[50];
};

#pragma pack(pop)
