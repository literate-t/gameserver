#include "Socket.h"
#include <iostream>
using namespace std;

Socket::Socket() {
	servSock = INVALID_SOCKET;
	clntSock = INVALID_SOCKET;
	ZeroMemory(sockBuf, BUF_SIZE);
}

Socket::~Socket() {
	WSACleanup();
}

bool Socket::InitSocket() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup() error");
		return false;
	}

	servSock = socket(AF_INET, SOCK_STREAM, 0);// IPPROTO_TCP);
	if (servSock == INVALID_SOCKET) {
		ErrorHandling("socket() error");
		return false;
	}
	cout << "server socket initializing success" << endl;
	return true;
}

void Socket::CloseSocket(SOCKET socket, bool isForce) {
	linger lin = { 0,0 };	// SO_DONTLINGER
	// isForse가 true이면 SO_LINGER, timeout = 0으로 설정하여
	// 강제 종료. 데이터 손실이 있을 수 있다
	if (isForce == true)
		lin.l_onoff = 1;
	// socket의 데이터 송수신을 모두 막기 
	shutdown(socket, SD_BOTH);
	// 소켓 옵션 설정
	setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&lin), sizeof(lin));
	// 소켓 연결 종료
	closesocket(socket);
	socket = INVALID_SOCKET;
}

bool Socket::BindandListen(USHORT port) {
	SOCKADDR_IN		servAddr;
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(servSock, reinterpret_cast<SOCKADDR*>(&servAddr), sizeof(servAddr)) != 0) {
		ErrorHandling("bind() error");
		return false;
	}

	if (listen(servSock, 5) != 0) {
		ErrorHandling("listen() error");
		return false;
	}
	cout << "server registry success" << endl;
	return true;
}

bool Socket::StartServer() {
	char str[BUF_SIZE];
	// 접속된 클라이언트 주소정보를 저장할 구조체 변수
	SOCKADDR_IN		clntAddr;
	int addrLen = sizeof(clntAddr);

	cout << "server started" << endl;

	if ((clntSock = accept(servSock, reinterpret_cast<SOCKADDR*>(&clntAddr), &addrLen)) == INVALID_SOCKET) {
		ErrorHandling("accept() error");
		return false;
	}
	sprintf_s(str, sizeof(str), "클라이언트 접속:IP(%s) SOCKET(%d)", inet_ntoa(clntAddr.sin_addr), clntSock);
	cout << str << endl;

	// 클라이언트가 메시지를 보내면 다시 클라이언트로 되돌려보낸다
	while (true) {
		int recvLen = recv(clntSock, sockBuf, BUF_SIZE, 0);
		if (recvLen == 0) {
			cout << "클라이언트와의 연결이 종료되었습니다." << endl;
			CloseSocket(clntSock);

			//StartServer();
			//return false;
			// 클라이언트 연결 종료
			CloseSocket(clntSock);
			CloseSocket(servSock);
			cout << "server terminated successfuly" << endl;
			return true;
		}

		else if (recvLen == -1) {
			cout << "StartServer() error recv() fail, ErrorCode:" << WSAGetLastError() << endl;
			CloseSocket(clntSock);
			StartServer();
			return false;
		}
		sockBuf[recvLen] = NULL;
		cout << "메시지수신: 수신 bytes[" << recvLen << "], 내용: [" << sockBuf << "]" << endl;
		int sendLen = send(clntSock, sockBuf, recvLen, 0);
		if (sendLen == -1) {
			cout << "StartServer() error send() fail, ErrorCode:" << WSAGetLastError() << endl;
			CloseSocket(clntSock);
			StartServer();
			return false;
		}
		cout << "메시지송신: 송신 bytes[" << sendLen << "], 내용: [" << sockBuf << "]" << endl;
	}

	//// 클라이언트 연결 종료
	//CloseSocket(clntSock);
	//CloseSocket(servSock);
	//cout << "server terminated successfuly" << endl;
	//return true;
}

bool Socket::Connect(const char* ip, USHORT port) {
	SOCKADDR_IN		servAddr;
	char			msg[BUF_SIZE];
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.S_un.S_addr = inet_addr(ip);
	servAddr.sin_port = htons(port);
	if (connect(servSock, reinterpret_cast<SOCKADDR*>(&servAddr), sizeof(servAddr)) == SOCKET_ERROR) {
		printf("connect() error\n");
		cout << "ErrorCode:" << WSAGetLastError() << endl;
		return false;
	}
	cout << "접속 성공" << endl;
	while (true) {
		cout << ">>";
		cin >> msg;
		if (!(_strcmpi(msg, "q") || _strcmpi(msg, "Q"))) break;
		
		int sendLen = send(servSock, msg, strlen(msg), 0);
		if (sendLen == -1) {
			ErrorHandling("Socket::Connect() send() fail");
			cout << "ErrorCode:" << WSAGetLastError() << endl;
			return false;
		}
		cout << "메시지송신: 송신bytes[" << sendLen << "], 내용:[" << msg << "]" << endl;

		int recvLen = recv(servSock, sockBuf, BUF_SIZE, 0);
		if (recvLen == 0) {
			cout << "클라이언트와의 연결이 종료되었습니다" << endl;
			CloseSocket(servSock);
			return false;
		}
		else if (recvLen == -1) {
			ErrorHandling("Connect() recv() fail");
			cout << "ErrorCode:" << WSAGetLastError() << endl;
			return false;
		}
		sockBuf[recvLen] = NULL;
		cout << "메시지수신: 수신bytes[" << recvLen << "], 내용:[" << sockBuf << "]" << endl;
	}
	CloseSocket(servSock);
	cout << "클라이언트 정상 종료" << endl;
	return true;
}

void Socket::ErrorHandling(const char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}