﻿#include "Socket.h"
#include <iostream>
using namespace std;

#define SERVER_IP		"127.0.0.1"
#define SERVER_PORT		8000

int main(int argc, const char* argv[])
{
	//if (argc > 2) {
	//	std::cout << "use proper parameter [option:[/server][/client]" << endl;
	//	return 0;
	//}
	Socket socket;

	////echo server
	//if (_strcmpi(argv[1], "/server") == 0) {
	//	socket.InitSocket();
	//	socket.BindandListen(SERVER_PORT);
	//	socket.StartServer();
	//	getchar();
	//}

	//echo client
	//else if (_strcmpi(argv[1], "/client") == 0) {
	//	socket.InitSocket();
	//	socket.Connect(SERVER_IP, SERVER_PORT);
	//}
	socket.InitSocket();
	socket.Connect(SERVER_IP, SERVER_PORT);

	//else {
	//	std::cout << "use proper parameter [option:[/server][/client]" << endl;
	//	return 0;
	//}
	return 0;
}