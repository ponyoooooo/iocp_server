#include "HuntingGroundManager.h"

HuntingGroundManager::~HuntingGroundManager() {
    for (auto room : m_groundList) {
        delete room;
    }
}

void HuntingGroundManager::Init(int32_t beginGroundNumber, int32_t maxGroundCount, int32_t maxGroundUserCount) {
    m_beginGroundNumber = beginGroundNumber;
    m_maxGroundCount = maxGroundCount;
    m_endGroundNumber = beginGroundNumber + maxGroundCount;

    m_groundList.resize(maxGroundCount);

    for (auto i = 0; i < maxGroundCount; ++i) {
        m_groundList[i] = new HuntingGround();
        m_groundList[i]->SendPacketFunc = SendPacketFunc;
        m_groundList[i]->Init(i + beginGroundNumber, maxGroundUserCount);
    }
}

uint32_t HuntingGroundManager::GetMaxGroundCount() const {
    return m_maxGroundCount;
}

uint16_t HuntingGroundManager::EnterUser(int32_t groundNumber, User* user) {
    auto ground = GetGroundByNumber(groundNumber);
    if (!ground) {
        return static_cast<uint16_t>(ERROR_CODE::GROUND_INVALID_INDEX);
    }

    auto retCode = static_cast<uint16_t>(ERROR_CODE::NONE);
    if (retCode = ground->EnterUser(user); retCode != static_cast<uint16_t>(ERROR_CODE::NONE))
    {
        return retCode;
    }

    ground->NotifyEnterUser(user);

    return retCode;
}

int16_t HuntingGroundManager::LeaveUser(int32_t roomNumber, User* user) {
    auto room = GetGroundByNumber(roomNumber);
    if (!room) {
        return static_cast<int16_t>(ERROR_CODE::GROUND_INVALID_INDEX);
    }
    user->SetDomainState(User::DOMAIN_STATE::LOGIN);
    room->LeaveUser(user);
    return static_cast<int16_t>(ERROR_CODE::NONE);
}

HuntingGround* HuntingGroundManager::GetGroundByNumber(int32_t number) {
    if (number < m_beginGroundNumber || number >= m_endGroundNumber) {
        return nullptr;
    }
    auto index = number - m_beginGroundNumber;
    return m_groundList[index];
}
