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
const std::uint16_t MAX_CLIENT = 100; // �� ���� ���� Ŭ���̾�Ʈ ��

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(1629);

	TextServer server;

	// ���� �ʱ�ȭ
	server.InitSocket();

	// ���ϰ� ���� �ּҸ� ���� �� ���
	server.BindandListen(SERVER_PORT);

	server.Run(MAX_CLIENT);

	std::cout << "�ƹ� Ű�� �������� \n";

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