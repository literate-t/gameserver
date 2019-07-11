#include "Socket.h"
#include "Protocol.h"
#include <iostream>

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
	std::cout << "server socket initializing success" << std::endl;
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
	std::cout << "server registry success" << std::endl;
	return true;
}

bool Socket::StartServer() {
	char str[BUF_SIZE];
	// ���ӵ� Ŭ���̾�Ʈ �ּ������� ������ ����ü ����
	SOCKADDR_IN		clntAddr;
	int addrLen = sizeof(clntAddr);

	std::cout << "server started" << std::endl;

	if ((clntSock = accept(servSock, reinterpret_cast<SOCKADDR*>(&clntAddr), &addrLen)) == INVALID_SOCKET) {
		ErrorHandling("accept() error");
		return false;
	}
	sprintf_s(str, sizeof(str), "Ŭ���̾�Ʈ ����:IP(%s) SOCKET(%d)", inet_ntoa(clntAddr.sin_addr), (int)clntSock);
	std::cout << str << std::endl;

	// Ŭ���̾�Ʈ�� �޽����� ������ �ٽ� Ŭ���̾�Ʈ�� �ǵ���������
	while (true) {
		int recvLen = recv(clntSock, sockBuf, BUF_SIZE, 0);
		if (recvLen == 0) {
			std::cout << "Ŭ���̾�Ʈ���� ������ ����Ǿ����ϴ�." << std::endl;
			CloseSocket(clntSock);

			//StartServer();
			//return false;
			// Ŭ���̾�Ʈ ���� ����
			CloseSocket(clntSock);
			CloseSocket(servSock);
			std::cout << "server terminated successfuly" << std::endl;
			return true;
		}

		else if (recvLen == -1) {
			std::cout << "StartServer() error recv() fail, ErrorCode:" << WSAGetLastError() << std::endl;
			CloseSocket(clntSock);
			StartServer();
			return false;
		}
		sockBuf[recvLen] = NULL;
		std::cout << "�޽�������: ���� bytes[" << recvLen << "], ����: [" << sockBuf << "]" << std::endl;
		int sendLen = send(clntSock, sockBuf, recvLen, 0);
		if (sendLen == -1) {
			std::cout << "StartServer() error send() fail, ErrorCode:" << WSAGetLastError() << std::endl;
			CloseSocket(clntSock);
			StartServer();
			return false;
		}
		std::cout << "�޽����۽�: �۽� bytes[" << sendLen << "], ����: [" << sockBuf << "]" << std::endl;
	}

	//// Ŭ���̾�Ʈ ���� ����
	//CloseSocket(clntSock);
	//CloseSocket(servSock);
	//std::cout << "server terminated successfuly" << std::endl;
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
		std::cout << "ErrorCode:" << WSAGetLastError() << std::endl;
		return false;
	}

	std::cout << "���� ����" << std::endl;
	PACKET packet;
	static int recvCount = 0;
	static int sendCount = 0;
	while (true) {
		std::cout << ">>";
		std::cin.getline(msg, 1024);
		if (!(_strcmpi(msg, "q") || _strcmpi(msg, "Q"))) break;
		ZeroMemory(&packet, sizeof(PACKET));
		CopyMemory(packet.msg, msg, strlen(msg));
		packet.length = strlen(msg) + PACKET_HEADER_SIZE;
		int sendLen = send(servSock, (char*)&packet, packet.length, 0);
		printf("send count:%d\n", ++sendCount);
		//int sendLen = send(servSock, msg, (int)strlen(msg), 0);
		if (sendLen == -1) {
			ErrorHandling("Socket::Connect() send() fail");
			std::cout << "ErrorCode:" << WSAGetLastError() << std::endl;
			return false;
		}
		std::cout << "�޽����۽�: �۽�bytes[" << sendLen << "], ����:[" << msg << "]" << std::endl;

		size_t strLen = strlen(msg);
		int recvLen = 0;
		while (true) {
			recvLen += recv(servSock, &sockBuf[recvLen], BUF_SIZE-1, 0);
			printf("recv count:%d\n", ++recvCount);
			if (recvLen >= strLen) break;
			if (recvLen == 0) {
				std::cout << "Ŭ���̾�Ʈ���� ������ ����Ǿ����ϴ�" << std::endl;
				CloseSocket(servSock);
				return false;
			}
			else if (recvLen == -1) {
				ErrorHandling("Connect() recv() fail");
				std::cout << "ErrorCode:" << WSAGetLastError() << std::endl;
				return false;
			}
		}
		sockBuf[recvLen] = NULL;
		std::cout << "�޽�������: ����bytes[" << recvLen << "], ����:[" << &sockBuf[PACKET_HEADER_SIZE] << "]" << std::endl;
	}
	CloseSocket(servSock);
	std::cout << "Ŭ���̾�Ʈ ���� ����" << std::endl;
	return true;
}

void Socket::ErrorHandling(const char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}