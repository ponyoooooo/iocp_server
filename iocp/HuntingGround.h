#pragma once

#include "UserManager.h"
#include "Packet.h"

#include <functional>
#include <list>

class HuntingGround
{
public:
    HuntingGround() = default;
    ~HuntingGround() = default;

    int32_t GetMaxUserCount() const;
    int32_t GetCurrentUserCount() const;
    int32_t GetGroundNumber() const;

    void Init(const int32_t m_groundNum, const int32_t m_maxUserCount);
    uint16_t EnterUser(User* user);
    void LeaveUser(User* leaveUser);

    void NotifyChat(int32_t clientIndex, const char* userID, const char* msg);
    void NotifyEnterUser(User* user);
    void NotifyLeaveUser(User* user);

    std::function<void(uint32_t, uint32_t, char*)> SendPacketFunc;

private:
    void SendToAllUser(const uint16_t dataSize, char* data, const int32_t passUserIndex, bool exceptMe) const;

    int32_t m_groundNum = -1;
    std::list<User*> m_userList;
    int32_t m_maxUserCount = 0;
    uint16_t m_currentUserCount = 0;
};