// User.h

#pragma once
#include <string>
#include <memory>
#include "Packet.h"
#include "Define.h"

class User
{
public:
    enum class DOMAIN_STATE
    {
        NONE = 0,
        LOGIN = 1,
        HuntingGround = 2
    };

    User();
    ~User();

    void Init(const int index);
    void Clear();
    int SetLogin(const std::string& userID);
    void EnterGround(int groundIndex);
    void SetDomainState(DOMAIN_STATE value);
    int GetCurrentGround() const;
    int GetNetConnIdx() const;
    std::string GetUserId() const;
    DOMAIN_STATE GetDomainState() const;
    void SetPacketData(const std::uint32_t dataSize, const char* pData);
    PacketInfo GetPacket();

private:
    int m_index = -1;
    int m_groundIndex = -1;
    std::string m_userID;
    bool m_isConfirm = false;
    std::string m_authToken;
    DOMAIN_STATE m_curDomainState = DOMAIN_STATE::NONE;
    std::uint32_t m_packetDataBufferWPos = 0;
    std::uint32_t m_packetDataBufferRPos = 0;
    std::unique_ptr<char[]> m_packetDataBuffer;

private:
    uint16_t m_level = 1;
    uint32_t m_exp = 0;
    uint32_t m_maxExp = 100;
    uint32_t m_hP = 50;
    uint32_t m_maxHP = 50;
    uint32_t m_mP = 50;
    uint32_t m_maxMP = 50;
    uint32_t m_attack = 10;
    uint32_t m_defense = 5;
    uint32_t m_gold = 100;
};
