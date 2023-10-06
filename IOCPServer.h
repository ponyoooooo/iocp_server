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

	//소켓을 초기화하는 함수
	bool InitSocket(const std::uint32_t maxIOWorkerThreadCount)
	{
		WSADATA wsaData;
		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData); // windows 버전 2.2 Winsock 라이브러리를 초기화하는 데 사용됩니다.
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

	//서버의 주소정보를 소켓과 연결시키고 접속 요청을 받기 위해 
	//소켓을 등록하는 함수
	bool BindandListen(const std::uint32_t uiBinPort)
	{
		SOCKADDR_IN	stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(uiBinPort); // server port
		//어떤 주소에서 들어오는 접속이라도 받아들이겠다.
		//보통 서버라면 이렇게 설정한다. 만약 한 아이피에서만 접속을 받고 싶다면
		//그 주소를 inet_addr함수를 이용해 넣으면 된다.
		stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		//위에서 지정한 서버 주소 정보와 cIOCompletionPort 소켓을 연결한다.
		int nRet = bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));
		if (nRet != 0)
		{
			std::cerr << std::format("Error: bind failed with code: {}\n", WSAGetLastError());
			return false;
		}

		//접속 요청을 받아들이기 위해 cIOCompletionPort소켓을 등록하고 
		//접속대기큐를 128개로 설정 한다.
		nRet = listen(m_listenSocket, 128);
		if (nRet != 0)
		{
			std::cerr << std::format("Error: listen failed with code: {}\n", WSAGetLastError());
			return false;
		}

		// completionPort 객체 생성 요청
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

	//접속 요청을 수락하고 메세지를 받아서 처리하는 함수
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

	// 스레드 종료
	void DestryThread()
	{
		// worker 스레드 종료
		m_isWorkerRunning = false;
		CloseHandle(m_iocpHandle);

		for (auto& th : m_IOWorkerThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		// accepter 스레드 종료
		m_isAccepterRunning = false;
		closesocket(m_listenSocket);

		if (m_accepterThread.joinable())
		{
			m_accepterThread.join();
		}

		// 클라이언트 객체 삭제
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
			m_clientInfos.back()->Initialize(i, m_iocpHandle); // m_idx 값을 설정
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
			// 스레드 생성 중에 오류 발생하는 경우
			std::cerr << "failed to create worker thread: " << e.what() << '\n';
			return false;
		}

		return true;
	}

	// Overlapped I/O 작업에 대한 완료 통보를 받아 처리하는 함수
	void WorkerThread()
	{
		// CompletionKey를 받을 포인터 변수
		ClientInfo* pClientInfo = nullptr;
		// 함수 호출 성공 여부
		bool	bSuccess = true;
		// Overlapped I/O 작업에서 전성된 데이터 크기
		DWORD dwIoSize = 0;
		// I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
		LPOVERLAPPED lpOverlapped = nullptr;

		while (m_isWorkerRunning)
		{
			//////////////////////////////////////////////////////
			//이 함수로 인해 쓰레드들은 WaitingThread Queue에
			//대기 상태로 들어가게 된다.
			//완료된 Overlapped I/O작업이 발생하면 IOCP Queue에서
			//완료된 작업을 가져와 뒤 처리를 한다.
			//그리고 PostQueuedCompletionStatus()함수에의해 사용자
			//메세지가 도착되면 쓰레드를 종료한다.
			//////////////////////////////////////////////////////

			bSuccess = GetQueuedCompletionStatus(
				m_iocpHandle,
				&dwIoSize, // 실제 전송된 바이트
				reinterpret_cast<PULONG_PTR>(&pClientInfo), // completionKey
				&lpOverlapped, // Overlapped IO 객체
				INFINITE // 대기할 시간
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

			// client 접속 종료시
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
			// 스레드 생성 중에 오류 발생하는 경우
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

	// 사용자의 접속을 받는 스레드
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

	// 클라이언트 정보 저장 구조체
	std::vector<std::unique_ptr<ClientInfo>>	m_clientInfos;
	std::queue<std::uint32_t>	m_availableClientIndices;

	// 클라이언트 접속을 받기 위한 리슨 소켓
	SOCKET m_listenSocket = INVALID_SOCKET;

	// io worker thread
	std::vector<std::thread> m_IOWorkerThreads;

	// Accept thread
	std::thread m_accepterThread;

	// CompletionPort 객체 핸들
	HANDLE m_iocpHandle = INVALID_HANDLE_VALUE;

	// 작업 스레드 동작 플래그
	bool m_isWorkerRunning = true;

	// 접속 스레드 동작 플래그
	bool m_isAccepterRunning = true;

	std::mutex m_clientIndicesMutex; // 스레드 안전을 위한 뮤텍스
};
