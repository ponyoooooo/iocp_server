// debug_new.cpp
// compile by using: cl /EHsc /W4 /D_DEBUG /MDd debug_new.cpp
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif

#include "TextServer.h"
#include <string>

const std::uint16_t SERVER_PORT = 8080;
const std::uint16_t MAX_CLIENT = 100; // 총 접속 가능 클라이언트 수

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(1629);

	TextServer server;

	// 소켓 초기화
	server.InitSocket();

	// 소켓과 서버 주소를 연결 및 등록
	server.BindandListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	std::cout << "아무 키나 누르세요 \n";

	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.End();

#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}