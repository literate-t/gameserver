#pragma once
#pragma comment(lib, "ws2_32.lib")
#define BUF_SIZE 1024
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Windows.h>
#include <process.h>

class Socket {
public:
	Socket();
	~Socket();

	/*server client common function*/
	// socket initializing
	bool InitSocket();
	// socket connect terminating
	void CloseSocket(SOCKET socket, bool IsForce = false);

	/*for server*/
	bool BindandListen(USHORT port);
	// accept
	bool StartServer();

	/*for client*/
	bool Connect(const char* ip, USHORT port);

	void ErrorHandling(const char* message);

private:
	SOCKET		servSock;
	SOCKET		clntSock;
	char		sockBuf[BUF_SIZE];
};