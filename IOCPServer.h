#pragma once

#include "Define.h"
#include "ClientInfo.h"

#include <iostream>
#include <vector>
#include <format>
#include <queue>


class IOCPServer
{
public:
	IOCPServer(void) {}
	virtual ~IOCPServer(void)
	{
		WSACleanup();
	}

	//������ �ʱ�ȭ�ϴ� �Լ�
	bool InitSocket(const std::uint32_t maxIOWorkerThreadCount)
	{
		WSADATA wsaData;
		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData); // windows ���� 2.2 Winsock ���̺귯���� �ʱ�ȭ�ϴ� �� ���˴ϴ�.
		if (nRet != 0)
		{
			std::cerr << std::format("Error: WSAStartup failed with code: {}\n", WSAGetLastError());
			return false;
		}

		m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, NULL, WSA_FLAG_OVERLAPPED);
		if (m_listenSocket == INVALID_SOCKET)
		{
			std::cerr << std::format("Error: WSASocket failed with code: {}\n", WSAGetLastError());
			return false;
		}

		m_maxIOWorkerThreadCount = maxIOWorkerThreadCount;

		std::cout << "socket init success" << '\n';

		return true;
	}

	//������ �ּ������� ���ϰ� �����Ű�� ���� ��û�� �ޱ� ���� 
	//������ ����ϴ� �Լ�
	bool BindandListen(const std::uint32_t uiBinPort)
	{
		SOCKADDR_IN	stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(uiBinPort); // server port
		//� �ּҿ��� ������ �����̶� �޾Ƶ��̰ڴ�.
		//���� ������� �̷��� �����Ѵ�. ���� �� �����ǿ����� ������ �ް� �ʹٸ�
		//�� �ּҸ� inet_addr�Լ��� �̿��� ������ �ȴ�.
		stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		//������ ������ ���� �ּ� ������ cIOCompletionPort ������ �����Ѵ�.
		int nRet = bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));
		if (nRet != 0)
		{
			std::cerr << std::format("Error: bind failed with code: {}\n", WSAGetLastError());
			return false;
		}

		//���� ��û�� �޾Ƶ��̱� ���� cIOCompletionPort������ ����ϰ� 
		//���Ӵ��ť�� 128���� ���� �Ѵ�.
		nRet = listen(m_listenSocket, 128);
		if (nRet != 0)
		{
			std::cerr << std::format("Error: listen failed with code: {}\n", WSAGetLastError());
			return false;
		}

		// completionPort ��ü ���� ��û
		m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, MAX_WORKERTHREAD);
		if (m_iocpHandle == nullptr)
		{
			std::cerr << std::format("Error: CreateIoCompletionPort failed with code: {}\n", GetLastError());
			return false;
		}

		auto hIOCPHandle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_listenSocket), m_iocpHandle, static_cast<std::uint32_t>(0), 0);
		if (m_iocpHandle == nullptr)
		{
			std::cerr << std::format("listen socket IOCP bind failed : {}", WSAGetLastError());
			return false;
		}

		std::cout << "server add success" << '\n';
		return true;
	}

	//���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
	bool StartServer(const std::uint32_t maxClientCount)
	{
		CreateClient(maxClientCount);

		bool bRet = CreateWorkerThread();
		if (bRet == false)
		{
			std::cerr << std::format("Error: CreateWorkerThread failed") << '\n';
			return false;
		}

		bRet = CreateAccepterThread();
		if (bRet == false)
		{
			std::cerr << std::format("Error: CreateAccepterThread failed") << '\n';
			return false;
		}

		std::cout << "server start success" << '\n';
		return true;
	}

	// ������ ����
	void DestryThread()
	{
		// worker ������ ����
		m_isWorkerRunning = false;
		CloseHandle(m_iocpHandle);

		for (auto& th : m_IOWorkerThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		// accepter ������ ����
		m_isAccepterRunning = false;
		closesocket(m_listenSocket);

		if (m_accepterThread.joinable())
		{
			m_accepterThread.join();
		}

		// Ŭ���̾�Ʈ ��ü ����
		for(auto& client : m_clientInfos)
		{
			if (client->IsConnected())
			{
				client->CloseSocket();
			}
		}
		m_clientInfos.clear();
	}

	bool SendMsg(const std::uint32_t sessionIndex, const std::uint32_t dataSize, const char* pData)
	{
		const auto pClient = GetClientInfo(sessionIndex);
		return pClient->SendMsg(pData, dataSize);
	}

	virtual void OnConnect(const std::uint32_t clientIndex_) {}
	virtual void OnClose(const std::uint32_t clientIndex_) {}
	virtual void OnReceive(const std::uint32_t clientIndex_, const std::uint32_t size_, char* pData_) {}

private:
	void CreateClient(const std::uint32_t maxClientCount)
	{
		for (std::uint32_t i = 0; i < maxClientCount; ++i)
		{
			m_clientInfos.emplace_back(std::make_unique<ClientInfo>());
			m_clientInfos.back()->Initialize(i, m_iocpHandle); // m_idx ���� ����
			m_availableClientIndices.push(i);
		}
	}

	bool CreateWorkerThread()
	{
		std::uint32_t uiThreadId = 0;
		try
		{
			for (unsigned int i = 0; i < MAX_WORKERTHREAD; ++i)
			{
				m_IOWorkerThreads.emplace_back([this]() { WorkerThread(); });
			}
		}
		catch (const std::system_error& e)
		{
			// ������ ���� �߿� ���� �߻��ϴ� ���
			std::cerr << "failed to create worker thread: " << e.what() << '\n';
			return false;
		}

		return true;
	}

	// Overlapped I/O �۾��� ���� �Ϸ� �뺸�� �޾� ó���ϴ� �Լ�
	void WorkerThread()
	{
		// CompletionKey�� ���� ������ ����
		ClientInfo* pClientInfo = nullptr;
		// �Լ� ȣ�� ���� ����
		bool	bSuccess = true;
		// Overlapped I/O �۾����� ������ ������ ũ��
		DWORD dwIoSize = 0;
		// I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
		LPOVERLAPPED lpOverlapped = nullptr;

		while (m_isWorkerRunning)
		{
			//////////////////////////////////////////////////////
			//�� �Լ��� ���� ��������� WaitingThread Queue��
			//��� ���·� ���� �ȴ�.
			//�Ϸ�� Overlapped I/O�۾��� �߻��ϸ� IOCP Queue����
			//�Ϸ�� �۾��� ������ �� ó���� �Ѵ�.
			//�׸��� PostQueuedCompletionStatus()�Լ������� �����
			//�޼����� �����Ǹ� �����带 �����Ѵ�.
			//////////////////////////////////////////////////////

			bSuccess = GetQueuedCompletionStatus(
				m_iocpHandle,
				&dwIoSize, // ���� ���۵� ����Ʈ
				reinterpret_cast<PULONG_PTR>(&pClientInfo), // completionKey
				&lpOverlapped, // Overlapped IO ��ü
				INFINITE // ����� �ð�
			);

			if (bSuccess && dwIoSize == 0 && lpOverlapped == nullptr)
			{
				m_isWorkerRunning = false;
				continue;
			}

			if (lpOverlapped == nullptr)
			{
				continue;
			}

			const auto pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);

			// client ���� �����
			if (bSuccess == false || (dwIoSize == 0 && pOverlappedEx->m_eOperation != IOOperation::ACCEPT))
			{
				//std::cout << std::format("socket={} close", static_cast<int>(pClientInfo->m_socketClient)) << '\n';
				pClientInfo->CloseSocket();
				continue;
			}

			if (pOverlappedEx->m_eOperation == IOOperation::ACCEPT)
			{
				pClientInfo = GetClientInfo(pOverlappedEx->m_sessionIdx);
				if (pClientInfo->AcceptCompletion())
				{
					OnConnect(pClientInfo->GetIdx());
				}
				else
				{
					CloseSocket(pClientInfo, true);
				}
			}
			else if(pOverlappedEx->m_eOperation == IOOperation::RECV)
			{
				OnReceive(pClientInfo->GetIdx(), dwIoSize, pClientInfo->GetRecvBuf());
				pClientInfo->BindRecv();
			}
			else if (pOverlappedEx->m_eOperation == IOOperation::SEND)
			{
				pClientInfo->SendCompleted(dwIoSize);
			}
			else
			{
				std::cout << std::format("socket{} exception", static_cast<int>(pClientInfo->GetIdx())) << '\n';
			}
		}
	}

	bool CreateAccepterThread()
	{
		try
		{
			m_accepterThread = std::thread([this]() { AccepterThread(); });
		}
		catch (const std::system_error& e)
		{
			// ������ ���� �߿� ���� �߻��ϴ� ���
			std::cerr << "failed to create accepter thread: " << e.what() << '\n';
			return false;
		}

		return true;
	}

	ClientInfo* GetClientInfo(const std::uint32_t sessionIndex)
	{
		return m_clientInfos[sessionIndex].get();
	}

	void SetClientInfo(const ClientInfo* clientInfo)
	{
		std::lock_guard<std::mutex> lock(m_clientIndicesMutex);
		m_availableClientIndices.push(clientInfo->GetIdx());
	}

	// ������� ������ �޴� ������
	void AccepterThread()
	{
		while (m_isAccepterRunning)
		{
			auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
			for (auto& client : m_clientInfos)
			{
				if (client->IsConnected())
				{
					continue;
				}

				if (static_cast<std::uint64_t>(curTimeSec) < client->GetLatestClosedTimeSec())
				{
					continue;
				}

				if (const auto diff = curTimeSec - client->GetLatestClosedTimeSec(); diff <= RE_USE_SESSION_WAIT_TIMESEC)
				{
					continue;
				}

				client->PostAccept(m_listenSocket);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}
	}

	void CloseSocket(ClientInfo* pClientInfo, bool bIsForce = false)
	{
		const auto clientIdx = pClientInfo->GetIdx();
		pClientInfo->CloseSocket(bIsForce);
		OnClose(clientIdx);
	}

	std::uint32_t m_maxIOWorkerThreadCount = 0;

	// Ŭ���̾�Ʈ ���� ���� ����ü
	std::vector<std::unique_ptr<ClientInfo>>	m_clientInfos;
	std::queue<std::uint32_t>	m_availableClientIndices;

	// Ŭ���̾�Ʈ ������ �ޱ� ���� ���� ����
	SOCKET m_listenSocket = INVALID_SOCKET;

	// io worker thread
	std::vector<std::thread> m_IOWorkerThreads;

	// Accept thread
	std::thread m_accepterThread;

	// CompletionPort ��ü �ڵ�
	HANDLE m_iocpHandle = INVALID_HANDLE_VALUE;

	// �۾� ������ ���� �÷���
	bool m_isWorkerRunning = true;

	// ���� ������ ���� �÷���
	bool m_isAccepterRunning = true;

	std::mutex m_clientIndicesMutex; // ������ ������ ���� ���ؽ�
};
