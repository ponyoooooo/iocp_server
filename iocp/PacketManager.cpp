#include "PacketManager.h"
#include "UserManager.h"

#include <utility>
#include <cstring>
#include <chrono>

PacketManager::PacketManager() = default;

void PacketManager::Init(const std::uint32_t maxClient)
{
    m_RecvFunctionDictionary[PACKET_ID::SYS_USER_CONNECT] = &PacketManager::ProcessUserConnect;
    m_RecvFunctionDictionary[PACKET_ID::SYS_USER_DISCONNECT] = &PacketManager::ProcessUserDisconnect;
    m_RecvFunctionDictionary[PACKET_ID::LOGIN_REQUEST] = &PacketManager::ProcessLogin;

    m_RecvFunctionDictionary[PACKET_ID::GROUND_ENTER_REQUEST] = &PacketManager::ProcessEnterGround;
    m_RecvFunctionDictionary[PACKET_ID::GROUND_LEAVE_REQUEST] = &PacketManager::ProcessLeaveGround;
    m_RecvFunctionDictionary[PACKET_ID::GROUND_CHAT_REQUEST] = &PacketManager::ProcessGroundChatMessage;

    CreateComponent(maxClient);
}

void PacketManager::CreateComponent(const std::uint32_t maxClient)
{
    m_userManager = std::make_unique<UserManager>();
    m_userManager->Init(maxClient);

    std::uint32_t startGroundNummber = 0;
    std::uint32_t maxGroundCount = 3;
    std::uint32_t maxGroundUserCount = 10;
    m_groundManager = std::make_unique<HuntingGroundManager>();
    m_groundManager->SendPacketFunc = SendPacketFunc;
    m_groundManager->Init(startGroundNummber, maxGroundCount, maxGroundUserCount);
}

bool PacketManager::Run()
{
    m_isRunProcessThread = true;
    m_processThread = std::thread([this]() { ProcessPacket(); });
    return true;
}

void PacketManager::End()
{
    m_isRunProcessThread = false;
    if (m_processThread.joinable())
    {
        m_processThread.join();
    }
}

void PacketManager::ReceivePacketData(const std::uint32_t clientIndex, const std::uint32_t size, char* pData)
{
    auto pUser = m_userManager->GetUserByConnIdx(clientIndex);
    pUser->SetPacketData(size, pData);
    EnqueuePacketData(clientIndex);
}

void PacketManager::ProcessPacket()
{
    while (m_isRunProcessThread)
    {
        bool isIdle = true;

        if (auto packetData = DequeuePacketData(); packetData.PacketId > PACKET_ID::SYS_END)
        {
            isIdle = false;
            ProcessRecvPacket(packetData.ClientIndex, packetData.PacketId, packetData.DataSize, packetData.pDataPtr);
        }

        if (auto packetData = DequeueSystemPacketData(); packetData.PacketId != PACKET_ID::NONE)
        {
            isIdle = false;
            ProcessRecvPacket(packetData.ClientIndex, packetData.PacketId, packetData.DataSize, packetData.pDataPtr);
        }

        if (isIdle)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void PacketManager::EnqueuePacketData(const std::uint32_t clientIndex)
{
    std::lock_guard<std::mutex> guard(m_lock);
    m_incomingPacketUserIndex.push_back(clientIndex);
}

PacketInfo PacketManager::DequeuePacketData()
{
    std::uint32_t userIndex = 0;
    {
        std::lock_guard<std::mutex> guard(m_lock);
        if (m_incomingPacketUserIndex.empty())
        {
            return PacketInfo();
        }

        userIndex = m_incomingPacketUserIndex.front();
        m_incomingPacketUserIndex.pop_front();
    }

    auto pUser = m_userManager->GetUserByConnIdx(userIndex);
    auto packetData = pUser->GetPacket();
    packetData.ClientIndex = userIndex;
    return packetData;
}

void PacketManager::PushSystemPacket(const PacketInfo& packet)
{
    std::lock_guard<std::mutex> guard(m_lock);
    m_systemPacketQueue.push_back(packet);
}

PacketInfo PacketManager::DequeueSystemPacketData()
{
    std::lock_guard<std::mutex> guard(m_lock);
    if (m_systemPacketQueue.empty())
    {
        return PacketInfo();
    }

    auto packetData = m_systemPacketQueue.front();
    m_systemPacketQueue.pop_front();
    return packetData;
}

void PacketManager::ProcessRecvPacket(const std::uint32_t clientIndex, const PACKET_ID packetId, const std::uint16_t packetSize, char* pPacket)
{
    auto iter = m_RecvFunctionDictionary.find(packetId);
    if (iter != m_RecvFunctionDictionary.end())
    {
        (this->*(iter->second))(clientIndex, packetSize, pPacket);
    }
}

void PacketManager::ProcessUserConnect(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket)
{
    printf("[ProcessUserConnect] clientIndex: %d\n", clientIndex);
    auto pUser = m_userManager->GetUserByConnIdx(clientIndex);
    pUser->Clear();
}

void PacketManager::ProcessUserDisconnect(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket)
{
    printf("[ProcessUserDisConnect] clientIndex: %d\n", clientIndex);
    ClearConnectionInfo(clientIndex);
}

void PacketManager::ProcessLogin(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket)
{
    if (LOGIN_REQUEST_PACKET_SZIE != packetSize)
    {
        return;
    }

    auto pLoginReqPacket = reinterpret_cast<LOGIN_REQUEST_PACKET*>(pPacket);

    auto pUserID = pLoginReqPacket->UserID;
    std::cout << std::format("requested user id = {}", pUserID) << '\n';

    LOGIN_RESPONSE_PACKET loginResPacket;
    loginResPacket.PacketId = PACKET_ID::LOGIN_RESPONSE;
    loginResPacket.PacketLength = sizeof(LOGIN_RESPONSE_PACKET);

    if (m_userManager->GetCurrentUserCount() >= m_userManager->GetMaxUserCount())
    {
        //접속자수가 최대수를 차지해서 접속불가
        loginResPacket.Result = static_cast<std::int16_t>(ERROR_CODE::LOGIN_USER_USED_ALL_OBJ);
        SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (char*)&loginResPacket);
        return;
    }

    //여기에서 이미 접속된 유저인지 확인하고, 접속된 유저라면 실패한다.
    if (m_userManager->FindUserIndexByID(pUserID) == -1)
    {
        m_userManager->AddUser(pUserID, clientIndex);
        loginResPacket.Result = (std::uint16_t)ERROR_CODE::NONE;
        SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (char*)&loginResPacket);
    }
    else
    {
        //접속중인 유저여서 실패를 반환한다.
        loginResPacket.Result = (std::uint16_t)ERROR_CODE::LOGIN_USER_ALREADY;
        SendPacketFunc(clientIndex, sizeof(LOGIN_RESPONSE_PACKET), (char*)&loginResPacket);
        return;
    }
}

void PacketManager::ClearConnectionInfo(const std::uint32_t clientIndex)
{
	const auto pReqUser = m_userManager->GetUserByConnIdx(clientIndex);
    if (User::DOMAIN_STATE::HUNTINGGROUND == pReqUser->GetDomainState())
    {
        m_groundManager->LeaveUser(pReqUser->GetCurrentGround(), pReqUser);
    }

    if (User::DOMAIN_STATE::NONE != pReqUser->GetDomainState())
    {
        m_userManager->DeleteUserInfo(pReqUser);
    }
}

void PacketManager::ProcessEnterGround(const std::uint32_t clientIndex, std::uint16_t packetSize, char* pPacket)
{
    UNREFERENCED_PARAMETER(packetSize);

    auto pGroundEnterReqPacket = reinterpret_cast<GROUND_ENTER_REQUEST_PACKET*>(pPacket);
    auto pReqUser = m_userManager->GetUserByConnIdx(clientIndex);

    if (!pReqUser)
    {
        return;
    }

    GROUND_ENTER_RESPONSE_PACKET groundEnterResPacket;
    groundEnterResPacket.PacketId = PACKET_ID::GROUND_ENTER_RESPONSE;
    groundEnterResPacket.PacketLength = sizeof(GROUND_ENTER_RESPONSE_PACKET);
    groundEnterResPacket.GroundNum = pGroundEnterReqPacket->GroundNumber;
    groundEnterResPacket.Result = m_groundManager->EnterUser(pGroundEnterReqPacket->GroundNumber, pReqUser);

    SendPacketFunc(clientIndex, sizeof(groundEnterResPacket), reinterpret_cast<char*>(&groundEnterResPacket));
}

void PacketManager::ProcessLeaveGround(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket)
{
    UNREFERENCED_PARAMETER(packetSize);
    UNREFERENCED_PARAMETER(pPacket);

    auto reqUser = m_userManager->GetUserByConnIdx(clientIndex);
    auto groundNum = reqUser->GetCurrentGround();

    GROUND_LEAVE_RESPONSE_PACKET groundLeaveResPacket;
    groundLeaveResPacket.PacketId = PACKET_ID::GROUND_LEAVE_RESPONSE;
    groundLeaveResPacket.PacketLength = sizeof(GROUND_LEAVE_RESPONSE_PACKET);
    groundLeaveResPacket.Result = m_groundManager->LeaveUser(groundNum, reqUser);

    SendPacketFunc(clientIndex, sizeof(groundLeaveResPacket), reinterpret_cast<char*>(&groundLeaveResPacket));
}

void PacketManager::ProcessGroundChatMessage(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket)
{
    UNREFERENCED_PARAMETER(packetSize);

    auto pGroundChatReqPacketet = reinterpret_cast<GROUND_CHAT_REQUEST_PACKET*>(pPacket);
    auto reqUser = m_userManager->GetUserByConnIdx(clientIndex);
    auto groundNum = reqUser->GetCurrentGround();

    auto pGround = m_groundManager->GetGroundByNumber(groundNum);
    if (!pGround)
    {
        GROUND_CHAT_RESPONSE_PACKET groundChatResPacket;
        groundChatResPacket.PacketId = PACKET_ID::GROUND_CHAT_RESPONSE;
        groundChatResPacket.PacketLength = sizeof(GROUND_CHAT_RESPONSE_PACKET);
        groundChatResPacket.Result = static_cast<std::int16_t>(ERROR_CODE::CHAT_GROUND_INVALID_GROUND_NUMBER);

        SendPacketFunc(clientIndex, sizeof(groundChatResPacket), reinterpret_cast<char*>(&groundChatResPacket));
        return;
    }

    GROUND_CHAT_RESPONSE_PACKET groundChatResPacket;
    groundChatResPacket.PacketId = PACKET_ID::GROUND_CHAT_RESPONSE;
    groundChatResPacket.PacketLength = sizeof(GROUND_CHAT_RESPONSE_PACKET);
    groundChatResPacket.Result = static_cast<std::int16_t>(ERROR_CODE::NONE);

    SendPacketFunc(clientIndex, sizeof(groundChatResPacket), reinterpret_cast<char*>(&groundChatResPacket));
    pGround->NotifyChat(clientIndex, reqUser->GetUserId().c_str(), pGroundChatReqPacketet->Message);
}

