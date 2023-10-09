#pragma once

#include <cstdint>
#include "Packet.h"

class UserInfo {
private:
    uint16_t m_Level;
    uint32_t m_Exp;
    uint32_t m_MaxExp;
    uint32_t m_HP;
    uint32_t m_MaxHP;
    uint32_t m_MP;
    uint32_t m_MaxMP;
    uint32_t m_Attack;
    uint32_t m_Defense;
    uint32_t m_Gold;
    Item m_Inventory[MAX_INVENTORY_ITEMS];

public:
    UserInfo() : m_Level(0), m_Exp(0), m_MaxExp(0), m_HP(0), m_MaxHP(0), m_MP(0), m_MaxMP(0),
        m_Attack(0), m_Defense(0), m_Gold(0) {}

    void SetDataFromLoginPacket(const LOGIN_RESPONSE_PACKET* packet) {
        m_Level = packet->Level;
        m_Exp = packet->Exp;
        m_MaxExp = packet->MaxExp;
        m_HP = packet->HP;
        m_MaxHP = packet->MaxHP;
        m_MP = packet->MP;
        m_MaxMP = packet->MaxMP;
        m_Attack = packet->Attack;
        m_Defense = packet->Defense;
        m_Gold = packet->Gold;

        /*for (int i = 0; i < MAX_INVENTORY_ITEMS; ++i) {
            m_Inventory[i] = packet.Inventory[i];
        }*/
    }

    void PrintCharacterInfo() {
        std::cout << "+---------------------------------------+" << std::endl;
        std::cout << " Level: " << m_Level << " | Exp: " << m_Exp << "/" << m_MaxExp << '\n';
        std::cout << " HP: " << m_HP << "/" << m_MaxHP << " | MP: " << m_MP << "/" << m_MaxMP;
        std::cout << '\n';
        std::cout << " Attack: " << m_Attack << " | Defense: " << m_Defense << '\n';
        std::cout << " Gold: " << m_Gold;
        std::cout << '\n';
        std::cout << "+---------------------------------------+" << '\n';
    }

    // 필요한 경우, 다른 멤버 함수나 getter, setter 함수들을 추가할 수 있습니다.
};