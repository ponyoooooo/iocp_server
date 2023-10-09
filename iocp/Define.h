#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock.lib")

#include <thread>
#include <algorithm>

#include "SpinLock.h"

inline const unsigned int MAX_WORKERTHREAD = 1 + (std::thread::hardware_concurrency() * 2);
inline const unsigned int SLEEP_SECONDS_3 = 3;
inline const unsigned int MAX_SOCKBUF = 256;
inline const unsigned int MAX_SEND_SOCKBUF = 4096;
inline const unsigned int RE_USE_SESSION_WAIT_TIMESEC = 3;
inline const std::uint32_t PACKET_DATA_BUFFER_SIZE = 8096;
inline const int MAX_USER_ID_LEN = 32;
inline const int MAX_USER_PW_LEN = 32;
inline const int MAX_CHAT_MSG_SIZE = 256;

enum class IOOperation
{
	NONE,
	ACCEPT,
	RECV,
	SEND
};

struct stOverlappedEx
{
	WSAOVERLAPPED	m_wsaOverlapped = {};
	WSABUF			m_wsaBuf = {};
	IOOperation		m_eOperation = IOOperation::NONE;
	std::uint32_t	m_sessionIdx = 0;
};

// 패킷 ID 열거형
enum class PACKET_ID : std::uint16_t
{
	NONE = 0,
	SYS_USER_CONNECT = 11,
	SYS_USER_DISCONNECT,
	SYS_END = 30,
	DB_END = 199,
	LOGIN_REQUEST = 201,
	LOGIN_RESPONSE,
	GROUND_ENTER_REQUEST = 206,
	GROUND_ENTER_RESPONSE,
	GROUND_LEAVE_REQUEST = 215,
	GROUND_LEAVE_RESPONSE,
	GROUND_CHAT_REQUEST = 221,
	GROUND_CHAT_RESPONSE,
	GROUND_CHAT_NOTIFY,
	GROUND_ENTER_NOTIFY,
	GROUND_LEAVE_NOTIFY,
};
