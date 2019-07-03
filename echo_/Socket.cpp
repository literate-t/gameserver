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
	// isForse�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ�
	// ���� ����. ������ �ս��� ���� �� �ִ�
	if (isForce == true)
		lin.l_onoff = 1;
	// socket�� ������ �ۼ����� ��� ���� 
	shutdown(socket, SD_BOTH);
	// ���� �ɼ� ����
	setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&lin), sizeof(lin));
	// ���� ���� ����
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
	// ���ӵ� Ŭ���̾�Ʈ �ּ������� ������ ����ü ����
	SOCKADDR_IN		clntAddr;
	int addrLen = sizeof(clntAddr);

	cout << "server started" << endl;

	if ((clntSock = accept(servSock, reinterpret_cast<SOCKADDR*>(&clntAddr), &addrLen)) == INVALID_SOCKET) {
		ErrorHandling("accept() error");
		return false;
	}
	sprintf_s(str, sizeof(str), "Ŭ���̾�Ʈ ����:IP(%s) SOCKET(%d)", inet_ntoa(clntAddr.sin_addr), clntSock);
	cout << str << endl;

	// Ŭ���̾�Ʈ�� �޽����� ������ �ٽ� Ŭ���̾�Ʈ�� �ǵ���������
	while (true) {
		int recvLen = recv(clntSock, sockBuf, BUF_SIZE, 0);
		if (recvLen == 0) {
			cout << "Ŭ���̾�Ʈ���� ������ ����Ǿ����ϴ�." << endl;
			CloseSocket(clntSock);

			//StartServer();
			//return false;
			// Ŭ���̾�Ʈ ���� ����
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
		cout << "�޽�������: ���� bytes[" << recvLen << "], ����: [" << sockBuf << "]" << endl;
		int sendLen = send(clntSock, sockBuf, recvLen, 0);
		if (sendLen == -1) {
			cout << "StartServer() error send() fail, ErrorCode:" << WSAGetLastError() << endl;
			CloseSocket(clntSock);
			StartServer();
			return false;
		}
		cout << "�޽����۽�: �۽� bytes[" << sendLen << "], ����: [" << sockBuf << "]" << endl;
	}

	//// Ŭ���̾�Ʈ ���� ����
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
	cout << "���� ����" << endl;
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
		cout << "�޽����۽�: �۽�bytes[" << sendLen << "], ����:[" << msg << "]" << endl;

		int recvLen = recv(servSock, sockBuf, BUF_SIZE, 0);
		if (recvLen == 0) {
			cout << "Ŭ���̾�Ʈ���� ������ ����Ǿ����ϴ�" << endl;
			CloseSocket(servSock);
			return false;
		}
		else if (recvLen == -1) {
			ErrorHandling("Connect() recv() fail");
			cout << "ErrorCode:" << WSAGetLastError() << endl;
			return false;
		}
		sockBuf[recvLen] = NULL;
		cout << "�޽�������: ����bytes[" << recvLen << "], ����:[" << sockBuf << "]" << endl;
	}
	CloseSocket(servSock);
	cout << "Ŭ���̾�Ʈ ���� ����" << endl;
	return true;
}

void Socket::ErrorHandling(const char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}