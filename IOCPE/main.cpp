#include "IOCP.h"
#include <iostream>
using namespace std;

#define SERVER_IP		"127.0.0.1"
#define SERVER_PORT		8000
int main(int argc, const char* argv[])
{
	IOCP socket;
	const int maxClient = 100;

	socket.Init(100);
	socket.BindAndListen(SERVER_PORT);
	socket.StartServer();
	getchar();
	socket.DestroyThread();
	return 0;
}