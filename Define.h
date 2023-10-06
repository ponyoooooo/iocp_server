#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock.lib")

#include <thread>
#include <algorithm>

inline const unsigned int MAX_WORKERTHREAD = 1 + (std::thread::hardware_concurrency() * 2);
inline const unsigned int SLEEP_SECONDS_3 = 3;
inline const unsigned int MAX_SOCKBUF = 256;
inline const unsigned int MAX_SEND_SOCKBUF = 4096;
inline const unsigned int RE_USE_SESSION_WAIT_TIMESEC = 3;

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
