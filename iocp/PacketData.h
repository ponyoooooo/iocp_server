#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <memory>

struct PacketData
{
    std::uint32_t m_sessionIndex = 0;
    [[nodiscard]] std::uint32_t GetSessionIndex() const;
    void SetSessionIndex(const std::uint32_t sessionIndex);

    std::uint32_t m_dataSize = 0;
    [[nodiscard]] std::uint32_t GetDataSize() const;
    void SetDataSize(const std::uint32_t dataSize);

    char* m_pPacketData = nullptr;
    [[nodiscard]] char* GetPacketData() const;
    void SetPacket(const PacketData& value);
    void SetPacketData(std::uint32_t sessionIndex_, std::uint32_t const dataSize_, const char* pPacketData_);
    void Release();
};
