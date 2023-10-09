#pragma once

#include "Packet.h"
#include "UserManager.h"
#include "HuntingGroundManager.h"

#include <unordered_map>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <memory>


class PacketManager {
public:
    PacketManager();
    ~PacketManager() = default;

    void Init(const std::uint32_t maxClient);
    bool Run();
    void End();
    void ReceivePacketData(const std::uint32_t clientIndex, const std::uint32_t size, char* pData);
    void PushSystemPacket(const PacketInfo& packet);
    std::function<void(std::uint32_t, std::uint16_t, char*)> SendPacketFunc;

private:
    void CreateComponent(const std::uint32_t maxClient);
    void ClearConnectionInfo(const std::uint32_t clientIndex);
    void EnqueuePacketData(const std::uint32_t clientIndex);
    PacketInfo DequeuePacketData();
    PacketInfo DequeueSystemPacketData();
    void ProcessPacket();
    void ProcessRecvPacket(const std::uint32_t clientIndex, const PACKET_ID packetId, const std::uint16_t packetSize, char* pPacket);
    void ProcessUserConnect(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket);
    void ProcessUserDisconnect(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket);
    void ProcessLogin(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket);
    void ProcessEnterGround(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket);
    void ProcessLeaveGround(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket);
    void ProcessGroundChatMessage(const std::uint32_t clientIndex, const std::uint16_t packetSize, char* pPacket);

    using PROCESS_RECV_PACKET_FUNCTION = void(PacketManager::*)(std::uint32_t, std::uint16_t, char*);
    std::unordered_map<PACKET_ID, PROCESS_RECV_PACKET_FUNCTION> m_RecvFunctionDictionary;

    std::unique_ptr<UserManager> m_userManager;
    std::unique_ptr<HuntingGroundManager> m_groundManager;

    std::function<void(int, char*)> m_sendMQDataFunc;

    bool m_isRunProcessThread = false;
    std::thread m_processThread;
    std::mutex m_lock;
    std::deque<std::uint32_t> m_incomingPacketUserIndex;
    std::deque<PacketInfo> m_systemPacketQueue;
};
