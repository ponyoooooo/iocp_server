#include "huntingGround.h"

int32_t HuntingGround::GetMaxUserCount() const {
    return m_maxUserCount;
}

int32_t HuntingGround::GetCurrentUserCount() const {
    return m_currentUserCount;
}

int32_t HuntingGround::GetGroundNumber() const {
    return m_groundNum;
}

void HuntingGround::Init(const int32_t groundNum, const int32_t maxUserCount) {
    m_groundNum = groundNum;
    m_maxUserCount = maxUserCount;
}

uint16_t HuntingGround::EnterUser(User* user) {
    if (m_currentUserCount >= m_maxUserCount) {
        return static_cast<uint16_t>(ERROR_CODE::ENTER_GROUND_FULL_USER);
    }

    m_userList.push_back(user);
    ++m_currentUserCount;

    user->EnterGround(m_groundNum);

    return static_cast<uint16_t>(ERROR_CODE::NONE);
}

void HuntingGround::LeaveUser(User* leaveUser) {
    m_userList.remove_if([leaveUserId = leaveUser->GetUserId()](User* user) {
        return leaveUserId == user->GetUserId();
        });

    NotifyLeaveUser(leaveUser);
}

void HuntingGround::NotifyChat(int32_t clientIndex, const char* userID, const char* msg) {
    GROUND_CHAT_NOTIFY_PACKET groundChatNtfyPkt;
    groundChatNtfyPkt.PacketId = PACKET_ID::GROUND_CHAT_NOTIFY;
    groundChatNtfyPkt.PacketLength = sizeof(groundChatNtfyPkt);

    std::memcpy(groundChatNtfyPkt.Msg, msg, sizeof(groundChatNtfyPkt.Msg));
    std::memcpy(groundChatNtfyPkt.UserID, userID, sizeof(groundChatNtfyPkt.UserID));
    SendToAllUser(sizeof(groundChatNtfyPkt), reinterpret_cast<char*>(&groundChatNtfyPkt), clientIndex, false);
}

void HuntingGround::NotifyEnterUser(User* enterUser)
{
    GROUND_USER_ENTER_NOTIFY_PACKET gUserNotifyPacket;
    gUserNotifyPacket.PacketId = PACKET_ID::GROUND_ENTER_NOTIFY;
    gUserNotifyPacket.PacketLength = sizeof(gUserNotifyPacket);
    enterUser->GetUserId().copy(gUserNotifyPacket.UserId, enterUser->GetUserId().size());

    SendToAllUser(sizeof(gUserNotifyPacket), reinterpret_cast<char*>(&gUserNotifyPacket), enterUser->GetNetConnIdx(), true);
}

void HuntingGround::NotifyLeaveUser(User* leaveUser)
{
    GROUND_USER_LEAVE_NOTIFY_PACKET gUserNotifyPacket;
    gUserNotifyPacket.PacketId = PACKET_ID::GROUND_LEAVE_NOTIFY;
    gUserNotifyPacket.PacketLength = sizeof(gUserNotifyPacket);
    leaveUser->GetUserId().copy(gUserNotifyPacket.UserId, leaveUser->GetUserId().size());

    SendToAllUser(sizeof(gUserNotifyPacket), reinterpret_cast<char*>(&gUserNotifyPacket), leaveUser->GetNetConnIdx(), true);
}

void HuntingGround::SendToAllUser(const uint16_t dataSize, char* data, const int32_t passUserIndex, bool exceptMe) const
{
    for (auto user : m_userList) {
        if (user == nullptr) {
            continue;
        }

        if (exceptMe && user->GetNetConnIdx() == passUserIndex) {
            continue;
        }

        SendPacketFunc(static_cast<uint32_t>(user->GetNetConnIdx()), static_cast<uint32_t>(dataSize), data);
    }
}
