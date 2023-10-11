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
	GROUND_MONSTER_LIST,
	GROUND_USER_LIST,
};

MENU menuStatus = MENU::MAIN;
std::uint16_t menuIdx = static_cast<uint16_t>(MENU::MAIN);

void PrintData()
{
	// 테두리 출력
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
	// 메뉴 출력
	std::cout << "\n\n";
	std::cout << "1. Login\n";
	std::cout << "2. Exit\n";
	std::cout << "Choose an option: ";
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

	// 로그인 패킷 생성 및 전송
	LOGIN_REQUEST_PACKET loginPacket;
	loginPacket.PacketId = PACKET_ID::LOGIN_REQUEST;
	loginPacket.PacketLength = LOGIN_REQUEST_PACKET_SZIE;
	strcpy_s(loginPacket.UserID, userId.c_str());
	strcpy_s(loginPacket.UserPW, userPw.c_str());

	send(clientSocket, reinterpret_cast<char*>(&loginPacket), LOGIN_REQUEST_PACKET_SZIE, 0);
}

void GroundEnter(std::uint16_t groundNumber)
{
	GROUND_ENTER_REQUEST_PACKET groundEnterPacket;
	groundEnterPacket.PacketId = PACKET_ID::GROUND_ENTER_REQUEST;
	groundEnterPacket.PacketLength = sizeof(GROUND_ENTER_REQUEST_PACKET);;
	groundEnterPacket.GroundNumber = groundNumber;

	send(clientSocket, reinterpret_cast<char*>(&groundEnterPacket), groundEnterPacket.PacketLength, 0);
}

void HuntingGroundMenu()
{
	std::cout << "\n=== 사냥터 선택 메뉴 ===" << '\n';
	std::cout << "0. 뒤로" << '\n';
	std::cout << "1. 초급 사냥터" << '\n';
	std::cout << "2. 중급 사냥터" << '\n';
	std::cout << "3. 고급 사냥터" << '\n';
	std::cout << "선택하세요: ";
}

void FightMenu()
{
	std::cout << "\n=== 사냥터 메뉴 ===" << '\n';
	std::cout << "0. 뒤로" << '\n';
	std::cout << "1. 채팅" << '\n';
	std::cout << "2. 유저 목록" << '\n';
	std::cout << "선택하세요: ";
}

void GameMenu()
{
	// 메뉴 출력
	std::cout << "\n\n";
	std::cout << "0. 뒤로\n";
	std::cout << "1. 인벤토리\n";
	std::cout << "2. 사냥터\n";
	std::cout << "3. 상점\n";
	std::cout << "선택하세요: ";
}

void DrawView()
{
	system("cls"); // 화면 지우기

	// 상단 데이터 출력
	PrintData();

	userInfo.PrintCharacterInfo();

	switch (menuStatus)
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
		HuntingGroundMenu();
		break;
	case MENU::GROUND_ENTER:
		FightMenu();
		break;
	case MENU::GROUND_USER_LIST:
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
	menuStatus = MENU::GAME;
	DrawView();
	dataMutex.unlock();
}

void GroundEnterResponse(char* recvBuffer)
{
	auto pGroundRepPacket = reinterpret_cast<GROUND_ENTER_RESPONSE_PACKET*>(recvBuffer);
	std::string message = "Ground Enter Rep Result: " + std::to_string(pGroundRepPacket->Result);

	if (pGroundRepPacket->Result == 0)
	{
		userInfo.SetGroundNum(pGroundRepPacket->GroundNum);
	}

	dataMutex.lock();
	if (dataQueue.size() >= DATA_LINES) {
		dataQueue.pop_front();
	}
	dataQueue.push_back(message);
	menuStatus = MENU::GROUND_ENTER;
	DrawView();
	dataMutex.unlock();
}

void GroundEnterNotice(char* recvBuffer)
{
	auto groundEnterNotice = reinterpret_cast<GROUND_USER_ENTER_NOTIFY_PACKET*>(recvBuffer);
	std::string message = "사냥터 입장 유저: " + std::string(groundEnterNotice->UserId);

	dataMutex.lock();
	if (dataQueue.size() >= DATA_LINES) {
		dataQueue.pop_front();
	}
	dataQueue.push_back(message);
	DrawView();
	dataMutex.unlock();
}

void GroundLeaveNotice(char* recvBuffer)
{
	auto groundEnterNotice = reinterpret_cast<GROUND_USER_ENTER_NOTIFY_PACKET*>(recvBuffer);
	std::string message = "사냥터 퇴장 유저: " + std::string(groundEnterNotice->UserId);

	dataMutex.lock();
	if (dataQueue.size() >= DATA_LINES) {
		dataQueue.pop_front();
	}
	dataQueue.push_back(message);
	DrawView();
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
			case PACKET_ID::GROUND_LEAVE_NOTIFY:
				GroundLeaveNotice(recvBuffer);
				break;
			default:
				break;
			}
		}
		else if (recvLen == 0 || WSAGetLastError() == WSAECONNRESET) {
			// 서버와의 연결이 끊어짐
			break;
		}
	}
}

int main()
{
	WSADATA wsaData;
	sockaddr_in serverAddr;

	// Winsock 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed." << std::endl;
		return -1;
	}

	// 소켓 생성
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "socket() failed." << std::endl;
		WSACleanup();
		return -1;
	}

	// 서버 정보 설정
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
	//serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버 IP
	serverAddr.sin_port = htons(8080); // 서버 포트

	// 서버에 연결
	if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "connect() failed." << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return -1;
	}

	std::thread recvThread(RecvThread, clientSocket);

	while (true) {
		system("cls"); // 화면 지우기

		// 상단 데이터 출력
		dataMutex.lock();
		DrawView();
		dataMutex.unlock();

		std::cin >> menuIdx;
		std::cin.ignore(); // 이전 입력에서 남아있는 개행 문자를 제거

		switch (menuStatus)
		{
		case MENU::MAIN:
			switch (menuIdx)
			{
			case 1:
				menuStatus = MENU::LOGIN;
				break;
			case 2:
				return false;
			}
			break;
		case MENU::GAME:
			switch (menuIdx)
			{
			case 1: // 인벤토리
				break;
			case 2: // 사냥터
				menuStatus = MENU::GROUND_SELECT;
				break;
			case 3: // 상점
				break;
			}
			break;
		case MENU::GROUND_SELECT:
			menuStatus = MENU::GAME;
			GroundEnter(menuIdx);
			break;
		}
	}

	recvThread.join();
	closesocket(clientSocket);
	WSACleanup();

	return 0;
}
