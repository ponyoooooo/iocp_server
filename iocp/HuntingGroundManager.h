#pragma once

#include "HuntingGround.h"
#include <vector>
#include <functional>

class HuntingGroundManager
{
public:
    HuntingGroundManager() = default;
    ~HuntingGroundManager();

    void Init(int32_t beginGroundNumber, int32_t maxGroundCount, int32_t maxGroundUserCount);
    uint32_t GetMaxGroundCount() const;
    uint16_t EnterUser(int32_t groundNumber, User* user);
    int16_t LeaveUser(int32_t groundNumber, User* user);
    HuntingGround* GetGroundByNumber(int32_t number);

    std::function<void(uint32_t, uint16_t, char*)> SendPacketFunc;

private:
    std::vector<HuntingGround*> m_groundList;
    int32_t m_beginGroundNumber = 0;
    int32_t m_endGroundNumber = 0;
    int32_t m_maxGroundCount = 0;
};
