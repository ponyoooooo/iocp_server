#pragma once

#include <format>
#include <iostream>
#include <queue>

#include "Define.h"


class ClientInfo
{
public:
	ClientInfo() = default;
	~ClientInfo()
	{
		if (m_socketClient != INVALID_SOCKET)
		{
			CloseSocket(true);
		}

		while (!m_sendDataQueue.empty()) {
			delete[] m_sendDataQueue.front()->m_wsaBuf.buf;
			delete m_sendDataQueue.front();
			m_sendDataQueue.pop();
		}
	}
	ClientInfo(const ClientInfo&) = delete;
	ClientInfo& operator=(const ClientInfo&) = delete;

	void Initialize(const std::uint32_t idx, HANDLE iocpHandle)
	{
		m_idx = idx;
		m_iocpHandle = iocpHandle;
		m_socketClient = INVALID_SOCKET;
		m_stRecvOverlappedEx = {};
		std::ranges::fill(m_recvBuf, 0);
		std::fill_n(m_sendBuf, MAX_SEND_SOCKBUF, 0);
		std::fill_n(m_sendingBuf, MAX_SEND_SOCKBUF, 0);
	}

	bool OnConnect(HANDLE iocpHandle, const SOCKET socket)
	{
		m_socketClient = socket;

		// I/O Completion Port 객체와 소켓을 연결
		if (BindIOCompletionPort(iocpHandle) == false)
		{
			return false;
		}

		return BindRecv();
	}

	void CloseSocket(const bool bIsForce = false)
	{
		struct linger stLinger = { 0, 0 }; // SO_DONTLINGER로 설정

		// bIsForce가 ture이면 SO_LINGER, timeout = 0로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을 수 있음
		if (bIsForce)
		{
			stLinger.l_onoff = 1;
		}

		// socketClose 소켓의 데이터 송수신을 모두 중단 시킨다.
		if (shutdown(m_socketClient, SD_BOTH) == SOCKET_ERROR)
		{
			std::cerr << std::format("Error: shutdown failed with code: {}\n", WSAGetLastError());
		}

		// 소켓 옵션을 설정.
		if (setsockopt(m_socketClient, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&stLinger), sizeof(stLinger)) == SOCKET_ERROR)
		{
			std::cerr << std::format("Error: setsockopt failed with code: {}\n", WSAGetLastError());
		}

		m_isConnect = 0;
		m_lastClostedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		// 소켓 연결을 종료
		if (closesocket(m_socketClient) == SOCKET_ERROR)
		{
			std::cerr << std::format("Error: closesocket failed with code: {}\n", WSAGetLastError());
		}

		m_socketClient = INVALID_SOCKET;
	}

	bool PostAccept(const SOCKET listenSock)
	{
		std::cout << std::format("postAccept client index : {}", GetIdx()) << '\n';

		m_lastClostedTimeSec = UINT32_MAX;

		m_socketClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (m_socketClient == INVALID_SOCKET)
		{
			std::cout << std::format("client socket WSASocket error:{}", GetLastError()) << '\n';
			return false;
		}

		std::memset(&m_stAcceptContextEx, 0, sizeof(m_stAcceptContextEx));

		DWORD bytes = 0;
		m_stAcceptContextEx.m_wsaBuf.len = 0;
		m_stAcceptContextEx.m_wsaBuf.buf = nullptr;
		m_stAcceptContextEx.m_eOperation = IOOperation::ACCEPT;
		m_stAcceptContextEx.m_sessionIdx = m_idx;

		if (AcceptEx(listenSock, m_socketClient, m_accpetBuf, 0, sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16, &bytes, reinterpret_cast<LPWSAOVERLAPPED>(&(m_stAcceptContextEx))))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				std::cerr << std::format("AcceptEx Error : ", GetLastError()) << '\n';
				return false;
			}
		}

		return true;
	}

	bool AcceptCompletion()
	{
		std::cout << std::format("AcceptCompletion : sessionIdx={}", m_idx) << '\n';

		if (OnConnect(m_iocpHandle, m_socketClient) == false)
		{
			return false;
		}

		SOCKADDR_IN stClientAddr;
		int nAddrLen = sizeof(SOCKADDR_IN);
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		std::cout << std::format("client connect info = IP:{}, SOCKET:{}", clientIP, static_cast<int>(m_socketClient)) << '\n';

		return true;
	}

	bool BindIOCompletionPort(HANDLE iocpHandle)
	{
		if (const HANDLE hIOCP = CreateIoCompletionPort(
			reinterpret_cast<HANDLE>(GetSocketClient()),
			iocpHandle,
			reinterpret_cast<ULONG_PTR>(this),
			0
		); hIOCP == INVALID_HANDLE_VALUE)
		{
			std::cerr << std::format("BindIOCompletionPort CreateIoCompletionPort err {}", GetLastError()) << '\n';
			return false;
		}

		return true;
	}

	// WSARecv Overlapped I/O 작업
	bool BindRecv()
	{
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		// Overlapped I/O 각 정보를 셋팅
		m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
		m_stRecvOverlappedEx.m_wsaBuf.buf = m_recvBuf;
		m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

		std::ranges::fill(m_recvBuf, 0);

		const int nRet = WSARecv(m_socketClient,
			&(m_stRecvOverlappedEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			&dwFlag,
			reinterpret_cast<LPWSAOVERLAPPED>(&(m_stRecvOverlappedEx)),
			nullptr);

		// socket_error 발생시 client 소켓 연결이 끊어진걸로 처리
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cerr << std::format("Error: WSARecv failed with code: {}\n", WSAGetLastError());
			return false;
		}

		return true;
	}

	// 1개의 스레드에서만 호출 해야함
	// WSASend Overlapped I/O 작업
	bool SendMsg(const char* pMsg, const std::uint32_t dataSize)
	{
		if (dataSize == 0)
		{
			return false;
		}

		auto sendOverlappedEx = new stOverlappedEx;
		sendOverlappedEx->m_wsaBuf.len = dataSize;
		sendOverlappedEx->m_wsaBuf.buf = new char[dataSize];
		std::memcpy(sendOverlappedEx->m_wsaBuf.buf, pMsg, dataSize);
		sendOverlappedEx->m_eOperation = IOOperation::SEND;

		std::lock_guard<std::mutex> guard(m_sendLock);

		m_sendDataQueue.push(sendOverlappedEx);

		if (m_sendDataQueue.size() == 1)
		{
			SendIO();
		}

		return true;
	}

	void SendIO()
	{
		const auto sendOverlappedEx = m_sendDataQueue.front();

		DWORD dwRecvNumBytes = 0;
		const int nRet = WSASend
		(
			m_socketClient,
			&(sendOverlappedEx->m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			reinterpret_cast<LPWSAOVERLAPPED>(sendOverlappedEx),
			nullptr
		);

		// socket err 발생시 socket close로 처리
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cerr << std::format("Error: WSASend failed with code: {}\n", WSAGetLastError());
			return;
		}

		return;
	}

	void SendCompleted(const std::uint32_t dataSize)
	{
		std::cout << std::format("send completed bytes = {}", dataSize) << '\n';

		std::lock_guard<std::mutex> guard(m_sendLock);

		delete[] m_sendDataQueue.front()->m_wsaBuf.buf;
		delete m_sendDataQueue.front();

		m_sendDataQueue.pop();

		if (m_sendDataQueue.empty() == false)
		{
			SendIO();
		}
	}

	bool SetSocketOption()
	{
		int opt = 1;
		if (setsockopt(m_socketClient, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(int)) == SOCKET_ERROR)
		{
			std::cerr << std::format("TCP_NODELAY error:{}", GetLastError()) << '\n';
			return false;
		}

		opt = 0;
		if (setsockopt(m_socketClient, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&opt), sizeof(int)) == SOCKET_ERROR)
		{
			std::cerr << std::format("SO_RCVBUF change error:{}", GetLastError());
			return false;
		}

		return false;
	}

	bool IsConnected() const { return m_socketClient != INVALID_SOCKET; }
	std::uint64_t GetLatestClosedTimeSec() const { return m_lastClostedTimeSec;  }

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
	[[nodiscard]] std::uint32_t GetIdx() const
	{
		return m_idx;
	}

	void SetIdx(const std::uint32_t idx)
	{
		m_idx = idx;
	}

	[[nodiscard]] char* GetRecvBuf()
	{
		return m_recvBuf;
	}

	void SetSocketClient(const SOCKET socketClient)
	{
		m_socketClient = socketClient;
	}

	[[nodiscard]] SOCKET GetSocketClient() const
	{
		return m_socketClient;
	}
};
