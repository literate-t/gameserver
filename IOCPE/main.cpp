#include "IOCP.h"
#include <iostream>
#include <thread>
#include "Main.h"
using namespace std;

#define SERVER_IP		"127.0.0.1"
#define SERVER_PORT		8000

int main(int argc, const char* argv[])
{
	const int maxClient = 100;
	IOCP socket;
	socket.Init(maxClient);
	socket.BindAndListen(SERVER_PORT);
	socket.StartServer();
	getchar();
	socket.DestroyThread();
	return 0;
}