#include <iostream>
#include <cstring>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include "Packet.h"
#include <deque>
#include <windows.h>
#include "UserInfo.h"

#pragma comment(lib, "ws2_32")

const int DATA_LINES = 10;
std::deque<std::string> dataQueue;
std::mutex dataMutex;
SOCKET clientSocket;
UserInfo userInfo;

enum class MENU : std::uint16_t
{
    MAIN = 0,
    LOGIN = 1,
    EXIT = 2,
    GAME = 3,
    GROUND_SELECT = 4,
    GROUND_ENTER = 5,
};

std::uint16_t menuIdx = static_cast<uint16_t>(MENU::MAIN);

void PrintData() 
{
    // �׵θ� ���
    std::cout << "+------------------------------------------------------------------------------+" << std::endl;
    for (const auto& line : dataQueue) {
        std::cout << "| " << line;
        for (size_t i = line.length(); i < 76; ++i) {
            std::cout << " ";
        }
        std::cout << " |" << std::endl;
    }
    std::cout << "+------------------------------------------------------------------------------+" << std::endl;
}

void MainMenu()
{
    // �޴� ���
    std::cout << "\n\n";
    std::cout << "1. Login\n";
    std::cout << "2. Exit\n";
    std::cout << "Choose an option: ";

    std::cin >> menuIdx;
    std::cin.ignore(); // ���� �Է¿��� �����ִ� ���� ���ڸ� ����
}

void LoginMenu()
{
    std::string userId, userPw;

    std::cout << "\n\n";
    std::cout << "Login Menu" << std::endl;
    std::cout << "---------" << std::endl;
    std::cout << "Enter User ID: ";
    std::getline(std::cin, userId);

    std::cout << "Enter Password: ";
    std::getline(std::cin, userPw);

    // �α��� ��Ŷ ���� �� ����
    LOGIN_REQUEST_PACKET loginPacket;
    loginPacket.PacketId = PACKET_ID::LOGIN_REQUEST;
    loginPacket.PacketLength = LOGIN_REQUEST_PACKET_SZIE;
    strcpy_s(loginPacket.UserID, userId.c_str());
    strcpy_s(loginPacket.UserPW, userPw.c_str());

    send(clientSocket, reinterpret_cast<char*>(&loginPacket), LOGIN_REQUEST_PACKET_SZIE, 0);

    menuIdx = static_cast<std::uint16_t>(MENU::GAME);
}

void GroundEnter(std::uint16_t groundNumber)
{
    GROUND_ENTER_REQUEST_PACKET groundEnterPacket;
    groundEnterPacket.PacketId = PACKET_ID::GROUND_ENTER_REQUEST;
    groundEnterPacket.PacketLength = sizeof(GROUND_ENTER_REQUEST_PACKET);;
    groundEnterPacket.GroundNumber = groundNumber;

    send(clientSocket, reinterpret_cast<char*>(&groundEnterPacket), groundEnterPacket.PacketLength, 0);
}

void PrintHuntingGroundMenu()
{
    std::uint16_t groundNum;

    std::cout << "\n=== ����� �޴� ===" << '\n';
    std::cout << "1. �ʱ� �����" << '\n';
    std::cout << "2. �߱� �����" << '\n';
    std::cout << "3. ��� �����" << '\n';
    std::cout << "�����ϼ���: ";
    std::cin >> groundNum;
    std::cin.ignore();

    GroundEnter(groundNum);

    menuIdx = static_cast<std::uint16_t>(MENU::GAME);
}

void GameMenu()
{
    // �޴� ���
    std::cout << "\n\n";
    std::cout << "0. �ڷ�\n";
    std::cout << "1. �κ��丮\n";
    std::cout << "2. �����\n";
    std::cout << "3. ����\n";
    std::cout << "�����ϼ���: ";

    int idx = 0;
    std::cin >> idx;
    std::cin.ignore();

    switch (idx)
    {
    case 1:
        break;
    case 2:
        menuIdx = static_cast<std::uint16_t>(MENU::GROUND_SELECT);
        break;
    case 3:
        break;

    }
}

void DrawView(const std::uint16_t menuIdx_ = static_cast<std::uint16_t>(MENU::MAIN))
{
    system("cls"); // ȭ�� �����

    // ��� ������ ���
    PrintData();

    switch (static_cast<MENU>(menuIdx_))
    {
    case MENU::MAIN:
        MainMenu();
        break;
    case MENU::LOGIN:
        LoginMenu();
        break;
    case MENU::GAME:
        GameMenu();
        break;
    case MENU::GROUND_SELECT:
        PrintHuntingGroundMenu();
        break;
    default:
        break;
    }
}

void LoginResponse(char* recvBuffer)
{
    auto pLoginResponse = reinterpret_cast<LOGIN_RESPONSE_PACKET*>(recvBuffer);
    std::string message = "Login Result: " + std::to_string(pLoginResponse->Result);

    dataMutex.lock();
    if (dataQueue.size() >= DATA_LINES) {
        dataQueue.pop_front();
    }
    dataQueue.push_back(message);
    userInfo.SetDataFromLoginPacket(pLoginResponse);
    menuIdx = static_cast<std::uint16_t>(MENU::GAME);
    DrawView(menuIdx);
    dataMutex.unlock();
}

void GroundEnterResponse(char* recvBuffer)
{
    auto pGroundRepPacket = reinterpret_cast<GROUND_ENTER_RESPONSE_PACKET*>(recvBuffer);
    std::string message = "Ground Rep Result: " + std::to_string(pGroundRepPacket->Result);

    dataMutex.lock();
    if (dataQueue.size() >= DATA_LINES) {
        dataQueue.pop_front();
    }
    dataQueue.push_back(message);
    menuIdx = static_cast<std::uint16_t>(MENU::GAME);
    DrawView(menuIdx);
    dataMutex.unlock();
}

void GroundEnterNotice(char* recvBuffer)
{
    auto groundEnterNotice = reinterpret_cast<GROUND_USER_ENTER_NOTIFY_PACKET*>(recvBuffer);
    std::string message = "����� ���� ����: " + std::string(groundEnterNotice->UserId);

    dataMutex.lock();
    if (dataQueue.size() >= DATA_LINES) {
        dataQueue.pop_front();
    }
    dataQueue.push_back(message);
    DrawView(menuIdx);
    dataMutex.unlock();
}

void RecvThread(SOCKET clientSocket) 
{
    while (true) {
        char recvBuffer[4096];
        int recvLen = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
        if (recvLen > 0) {
            auto pHeader = reinterpret_cast<PACKET_HEADER*>(recvBuffer);
            switch (pHeader->PacketId)
            {
            case PACKET_ID::LOGIN_RESPONSE:
                LoginResponse(recvBuffer);
                break;
            case PACKET_ID::GROUND_ENTER_RESPONSE:
                GroundEnterResponse(recvBuffer);
                break;
            case PACKET_ID::GROUND_ENTER_NOTIFY:
                GroundEnterNotice(recvBuffer);
                break;
            default:
                break;
            }
        }
        else if (recvLen == 0 || WSAGetLastError() == WSAECONNRESET) {
            // �������� ������ ������
            break;
        }
    }
}

int main() 
{
    WSADATA wsaData;
    sockaddr_in serverAddr;

    // Winsock �ʱ�ȭ
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return -1;
    }

    // ���� ����
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed." << std::endl;
        WSACleanup();
        return -1;
    }

    // ���� ���� ����
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    //serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // ���� IP
    serverAddr.sin_port = htons(8080); // ���� ��Ʈ

    // ������ ����
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "connect() failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    std::thread recvThread(RecvThread, clientSocket);

    while (true) {
        system("cls"); // ȭ�� �����

        // ��� ������ ���
        dataMutex.lock();
        DrawView(menuIdx);
        dataMutex.unlock();
    }

    recvThread.join();
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
