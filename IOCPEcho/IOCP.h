#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <process.h>
#include <stdio.h>
#include <WS2tcpip.h>

#define MAX_SOCKBUF			1024
#define MAX_CLIENT			100
#define MAX_WORKERTHREAD	4

enum eOPERATION {
	OP_RECV,
	OP_SEND
};

struct OVERLAPPEDEX {
	WSAOVERLAPPED	wasOverlapped;
	SOCKET			socketClient;
	WSABUF			wsaBuf;
	char			buf[MAX_SOCKBUF];
	eOPERATION		op;
};

typedef struct _CLIENTINFO {
	SOCKET			socket;
	OVERLAPPEDEX	recvOverlappedEx;
	OVERLAPPEDEX	sendOverlappedEx;

	_CLIENTINFO()
	{
		ZeroMemory(&recvOverlappedEx, sizeof(OVERLAPPEDEX));
		ZeroMemory(&sendOverlappedEx, sizeof(OVERLAPPEDEX));
		socket = INVALID_SOCKET;
	}
}CLIENTINFO, *PCLIENTINFO;
class IOCP
{
private:
	// Ŭ���̾�Ʈ ���� ���� 
	CLIENTINFO*		m_pClientInfo;
	// Ŭ���̾�Ʈ ������ �ޱ� ���� ���� ����
	SOCKET			m_socketListen;
	// ���ӵǾ� �ִ� Ŭ���̾�Ʈ ��
	int				m_nClientCnt;
	// �۾� ������ �ڵ�(WaitingThread Queue�� �� ������)
	HANDLE			m_hWorkerThread[MAX_WORKERTHREAD];
	// ���� ������ �ڵ�
	HANDLE			m_hAccepterThread;
	// CompletionPort ��ü �ڵ�
	HANDLE			m_hIOCP;
	// �۾� ������ ���� �÷���
	bool			m_bWorkerRun;
	// ���� ������ ���� �÷���
	bool			m_bAccepterRun;
	// ���� ����
	char			m_socketBuf[1024];

public:
	IOCP();
	~IOCP();

	//----���� Ŭ���̾�Ʈ ����----//
	bool InitSocket();
	void CloseSocket(PCLIENTINFO& ClientInfo, bool bIsForce = false);

	//----����----//
	bool BindAndListen(const int& port);
	bool StartServer();
	// WaitingThread Queue���� ����� ������ ����
	bool CreateWorkerThread();
	// Accept ��û�� ó���� ������ ����
	bool CreateAccepterThread();
	// ������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ
	CLIENTINFO* GetEmptyClientInfo();
	// CompletionPort ��ü, ����, CompletionKey�� ����
	bool BindIOCompletionPort(PCLIENTINFO& clientInfo);
	// WSARecv Overlapped I/O �۾�
	bool RecvMsg(PCLIENTINFO pClientInfo);
	// WSASend Overlapped I/O �۾�
	bool SendMsg(PCLIENTINFO pClientInfo, const char* msg, const int len);
	// �Ϸ�� Overlapped I/O ó�� ������
	void WorkerThread();
	// ����� ���� �޴� ������
	void AccepterThread();
	void DestroyThread();
};

