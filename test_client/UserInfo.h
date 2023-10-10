#pragma once

#include <cstdint>
#include "Packet.h"

class UserInfo {
private:
    uint16_t m_groundNum = 0;

public:
    [[nodiscard]] uint16_t GetGroundNum() const
    {
	    return m_groundNum;
    }

    void SetGroundNum(const uint16_t groundNum)
    {
	    m_groundNum = groundNum;
    }

private:
    uint16_t m_level;
    uint32_t m_exp;
    uint32_t m_maxExp;
    uint32_t m_hp;
    uint32_t m_maxHp;
    uint32_t m_mp;
    uint32_t m_maxMp;
    uint32_t m_attack;
    uint32_t m_defense;
    uint32_t m_gold;
    Item m_inventory[MAX_INVENTORY_ITEMS];

public:
    UserInfo() : m_groundNum(0), m_level(0), m_exp(0), m_maxExp(0), m_hp(0), m_maxHp(0), m_mp(0), m_maxMp(0),
        m_attack(0), m_defense(0), m_gold(0) {}

    void SetDataFromLoginPacket(const LOGIN_RESPONSE_PACKET* packet) {
        m_level = packet->Level;
        m_exp = packet->Exp;
        m_maxExp = packet->MaxExp;
        m_hp = packet->HP;
        m_maxHp = packet->MaxHP;
        m_mp = packet->MP;
        m_maxMp = packet->MaxMP;
        m_attack = packet->Attack;
        m_defense = packet->Defense;
        m_gold = packet->Gold;

        /*for (int i = 0; i < MAX_INVENTORY_ITEMS; ++i) {
            m_inventory[i] = packet.Inventory[i];
        }*/
    }

    void PrintCharacterInfo() {
        if (0 == m_groundNum)
        {
            return;
        }

        std::cout << "+---------------------------------------+" << std::endl;
        std::cout << " 사냥터: " << m_groundNum << '\n';
        std::cout << " Level: " << m_level << " | Exp: " << m_exp << "/" << m_maxExp << '\n';
        std::cout << " HP: " << m_hp << "/" << m_maxHp << " | MP: " << m_mp << "/" << m_maxMp;
        std::cout << '\n';
        std::cout << " Attack: " << m_attack << " | Defense: " << m_defense << '\n';
        std::cout << " Gold: " << m_gold;
        std::cout << '\n';
        std::cout << "+---------------------------------------+" << '\n';
    }

    // 필요한 경우, 다른 멤버 함수나 getter, setter 함수들을 추가할 수 있습니다.
};