#pragma once
#include <unordered_map>
#include <vector>
#include <string>

#include "ErrorCode.h"
#include "User.h"

class UserManager
{
public:
    UserManager() = default;
    ~UserManager() = default;

    void Init(int m_maxUserCount);
    int GetCurrentUserCount() const;
    int GetMaxUserCount() const;
    void IncreaseUserCount();
    void DecreaseUserCount();
    ERROR_CODE AddUser(const std::string& userID, int clientIndex);
    int FindUserIndexByID(const std::string& userID) const;
    void DeleteUserInfo(User* user);
    User* GetUserByConnIdx(int clientIndex) const;

private:
    int m_MaxUserCount = 0;
    int m_CurrentUserCount = 0;
    std::vector<std::unique_ptr<User>> m_UserObjPool;
    std::unordered_map<std::string, int> m_UserIDDictionary;
};
