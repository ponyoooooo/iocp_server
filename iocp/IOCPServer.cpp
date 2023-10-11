#include "IOCPServer.h"

IOCPServer::IOCPServer(void)
    : m_maxIOWorkerThreadCount(0),
    m_listenSocket(INVALID_SOCKET),
    m_iocpHandle(INVALID_HANDLE_VALUE),
    m_isWorkerRunning(true),
    m_isAccepterRunning(true)
{}

IOCPServer::~IOCPServer(void)
{
    WSACleanup();
}

bool IOCPServer::InitSocket(const std::uint32_t maxIOWorkerThreadCount)
{
    WSADATA wsaData;
    int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
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

bool IOCPServer::BindandListen(const std::uint32_t uiBinPort)
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

bool IOCPServer::StartServer(const std::uint32_t maxClientCount)
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

void IOCPServer::DestryThread()
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
    for (auto& client : m_clientInfos)
    {
        if (client->IsConnected())
        {
            client->CloseSocket();
        }
    }
    m_clientInfos.clear();
}

bool IOCPServer::SendMsg(const std::uint32_t sessionIndex, const std::uint32_t dataSize, const char* pData)
{
    const auto pClient = GetClientInfo(sessionIndex);
    return pClient->SendMsg(pData, dataSize);
}

void IOCPServer::CreateClient(const std::uint32_t maxClientCount)
{
    for (std::uint32_t i = 0; i < maxClientCount; ++i)
    {
        m_clientInfos.emplace_back(std::make_unique<ClientInfo>());
        m_clientInfos.back()->Initialize(i, m_iocpHandle); // m_idx ���� ����
        AddClientIdxToAcceptQueue(i);
        //m_availableClientIndices.push(i);
    }
}

bool IOCPServer::CreateWorkerThread()
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

void IOCPServer::WorkerThread()
{
    constexpr int MAX_IOCP_EVENTS = 64; // �� ���� ó���� �� �ִ� �ִ� I/O �Ϸ� �̺�Ʈ ��
    OVERLAPPED_ENTRY iocpEvents[MAX_IOCP_EVENTS];
    ULONG removedEntries = 0;
   
    ClientInfo* pClientInfo = nullptr;  // CompletionKey�� ���� ������ ����
    bool	bSuccess = true;  // �Լ� ȣ�� ���� ����
    DWORD dwIoSize = 0; // Overlapped I/O �۾����� ������ ������ ũ��
    LPOVERLAPPED lpOverlapped = nullptr; // I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������

    while (m_isWorkerRunning)
    {
        bSuccess = GetQueuedCompletionStatusEx(
            m_iocpHandle,
            iocpEvents,
            MAX_IOCP_EVENTS,
            &removedEntries,
            INFINITE,
            FALSE
        );

        if (bSuccess == false)
        {
            std::cout << std::format("GetQueuedCompletionStatusEx failed : {}", GetLastError()) << '\n';
            continue;
        }

        for (ULONG i = 0; i < removedEntries; ++i)
        {
            dwIoSize = iocpEvents[i].dwNumberOfBytesTransferred;
            lpOverlapped = iocpEvents[i].lpOverlapped;
            pClientInfo = reinterpret_cast<ClientInfo*>(iocpEvents[i].lpCompletionKey);

            if (lpOverlapped == nullptr)
            {
                continue;
            }

            const auto pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);
            
            // client ���� �����
            if (bSuccess == false || (dwIoSize == 0 && pOverlappedEx->m_eOperation != IOOperation::ACCEPT))
            {
                //std::cout << std::format("socket={} close", static_cast<int>(pClientInfo->m_socketClient)) << '\n';
                CloseSocket(pClientInfo, true);
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
            else if (pOverlappedEx->m_eOperation == IOOperation::RECV)
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
}

bool IOCPServer::CreateAccepterThread()
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

ClientInfo* IOCPServer::GetClientInfo(const std::uint32_t sessionIndex) const
{
    return m_clientInfos[sessionIndex].get();
}

void IOCPServer::SetClientInfo(const ClientInfo* clientInfo)
{
    std::lock_guard<std::mutex> lock(m_clientIndicesMutex);
    //m_availableClientIndices.push(clientInfo->GetIdx());
}

void IOCPServer::AddClientIdxToAcceptQueue(std::uint16_t clientIdx)
{
    {
        std::lock_guard<std::mutex> lock(m_acceptClientQueueMutex);
        m_acceptClientIdxQueue.push(clientIdx);
    }
    m_condition.notify_one();
}

void IOCPServer::AccepterThread()
{
    const size_t batchSize = 10;
    while (m_isAccepterRunning)
    {
        size_t count = 0;
        {
            std::unique_lock<std::mutex> lock(m_acceptClientQueueMutex);
            m_condition.wait(lock, [this] { return !m_acceptClientIdxQueue.empty(); });

            while (!m_acceptClientIdxQueue.empty() && count < batchSize)
            {
	            const auto clientIdx = m_acceptClientIdxQueue.front();
                m_acceptClientIdxQueue.pop();
                GetClientInfo(clientIdx)->PostAccept(m_listenSocket);
                ++count;
            }
        }
    }
}

void IOCPServer::CloseSocket(ClientInfo* pClientInfo, bool bIsForce)
{
    const auto clientIdx = pClientInfo->GetIdx();
    pClientInfo->CloseSocket(bIsForce);
    AddClientIdxToAcceptQueue(pClientInfo->GetIdx());
    OnClose(clientIdx);
}