#pragma once

#include "Define.h"
#include "ClientInfo.h"

#include <iostream>
#include <vector>
#include <format>
#include <queue>
#include <thread>
#include <mutex>

class IOCPServer
{
public:
	IOCPServer(void);
	virtual ~IOCPServer(void);

	bool InitSocket();
	bool BindandListen(const std::uint32_t uiBinPort);
	bool StartServer(const std::uint32_t maxClientCount);
	void DestryThread();
	bool SendMsg(const std::uint32_t sessionIndex, const std::uint32_t dataSize, const char* pData);

	virtual void OnConnect(const std::uint32_t clientIndex) {};
	virtual void OnClose(const std::uint32_t clientIndex) {};
	virtual void OnReceive(const std::uint32_t clientIndex, const std::uint32_t size, char* pData) {};

private:
	void CreateClient(const std::uint32_t maxClientCount);
	bool CreateWorkerThread();
	void WorkerThread();
	bool CreateAccepterThread();
	ClientInfo* GetClientInfo(const std::uint32_t sessionIndex) const;
	void SetClientInfo(const ClientInfo* clientInfo);
	void AddClientIdxToAcceptQueue(std::uint16_t clientIdx);
	void AccepterThread();
	void CloseSocket(ClientInfo* pClientInfo, bool bIsForce = false);

	std::queue<std::uint16_t> m_acceptClientIdxQueue;
	std::mutex m_acceptClientQueueMutex;
	std::condition_variable m_condition;
	std::uint32_t m_maxIOWorkerThreadCount;
	std::vector<std::unique_ptr<ClientInfo>> m_clientInfos;
	//std::queue<std::uint32_t> m_availableClientIndices;
	SOCKET m_listenSocket;
	std::vector<std::thread> m_IOWorkerThreads;
	std::thread m_accepterThread;
	HANDLE m_iocpHandle;
	bool m_isWorkerRunning;
	bool m_isAccepterRunning;
	std::mutex m_clientIndicesMutex;
};
