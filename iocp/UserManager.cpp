#include "UserManager.h"


void UserManager::Init(int m_maxUserCount)
{
    m_MaxUserCount = m_maxUserCount;
    m_UserObjPool.resize(m_MaxUserCount);

    for (int i = 0; i < m_MaxUserCount; ++i)
    {
        m_UserObjPool[i] = std::make_unique<User>();
        m_UserObjPool[i]->Init(i);
    }
}

int UserManager::GetCurrentUserCount() const
{
    return m_CurrentUserCount;
}

int UserManager::GetMaxUserCount() const
{
    return m_MaxUserCount;
}

void UserManager::IncreaseUserCount()
{
    m_CurrentUserCount++;
}

void UserManager::DecreaseUserCount()
{
    if (m_CurrentUserCount > 0)
    {
        m_CurrentUserCount--;
    }
}

ERROR_CODE UserManager::AddUser(const std::string& userID, int clientIndex)
{
    m_UserObjPool[clientIndex]->SetLogin(userID.c_str());
    m_UserIDDictionary[userID] = clientIndex;

    return ERROR_CODE::NONE;
}

int UserManager::FindUserIndexByID(const std::string& userID) const
{
    if (auto res = m_UserIDDictionary.find(userID); res != m_UserIDDictionary.end())
    {
        return res->second;
    }

    return -1;
}

void UserManager::DeleteUserInfo(User* user)
{
    m_UserIDDictionary.erase(user->GetUserId());
    user->Clear();
}

User* UserManager::GetUserByConnIdx(int clientIndex) const
{
    return m_UserObjPool[clientIndex].get();
}
