#pragma once

#include <format>
#include <iostream>
#include <queue>

#include "Define.h"


class ClientInfo
{
public:
	ClientInfo();
	~ClientInfo();
	ClientInfo(const ClientInfo&) = delete;
	ClientInfo& operator=(const ClientInfo&) = delete;

	void Initialize(const std::uint32_t idx, HANDLE iocpHandle);
	bool OnConnect(HANDLE iocpHandle, const SOCKET socket);
	void CloseSocket(const bool bIsForce = false);
	bool PostAccept(const SOCKET listenSock);
	bool AcceptCompletion();
	bool BindIOCompletionPort(HANDLE iocpHandle);
	bool BindRecv();
	bool SendMsg(const char* pMsg, const std::uint32_t dataSize);
	void SendIO();
	void SendCompleted(const std::uint32_t dataSize);
	bool SetSocketOption();
	bool IsConnected() const;
	std::uint64_t GetLatestClosedTimeSec() const;

private:
	HANDLE m_iocpHandle = INVALID_HANDLE_VALUE;
	std::uint32_t	m_idx = 0;
	std::uint32_t m_sendPos = 0;
	std::uint64_t m_isConnect = 0;
	std::uint64_t m_lastClostedTimeSec = 0;
	std::mutex m_sendLock;
	bool m_isSending = false;
	char m_sendBuf[MAX_SEND_SOCKBUF];
	char m_sendingBuf[MAX_SEND_SOCKBUF];

	SOCKET m_socketClient = INVALID_SOCKET;	// client와 연결되는 소켓
	stOverlappedEx	m_stRecvOverlappedEx{};	// recv overlapped I/O 작업을 위한 변수
	stOverlappedEx m_stAcceptContextEx{};
	std::queue<stOverlappedEx*> m_sendDataQueue;

	char m_accpetBuf[64];
	char m_recvBuf[MAX_SOCKBUF] = { 0, };

public:
	[[nodiscard]] std::uint32_t GetIdx() const;
	void SetIdx(const std::uint32_t idx);
	[[nodiscard]] char* GetRecvBuf();
	void SetSocketClient(const SOCKET socketClient);
	[[nodiscard]] SOCKET GetSocketClient() const;
};
