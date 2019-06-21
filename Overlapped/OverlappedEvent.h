#pragma once
#include "Precompile.h"
#define MAX_SOCKBUF 1024

enum eOperation {
	OP_RECV,
	OP_SEND
};

struct OverlappedEx {
	WSAOVERLAPPED	wsaOverlapped;
	int				idx;	// ����ü �迭 �ε���
	WSABUF			wsaBuf;	// Overlapped I/O �۾� ����
	char			buf[MAX_SOCKBUF];
	eOperation		op;		// ���� ����
};

// Ŭ���̾�Ʈ ������ ��� ���� ����ü
// 0��° �迭�� ���̷�(?) ������ ������ ���ο� ������ �̺�Ʈ ������ ����
// WSAWaitForMultipleEvents�� �ٽ� �ɾ��ش�
struct ClientInfo {
	// Client�� ����Ǵ� ����
	SOCKET			sockClient[WSA_MAXIMUM_WAIT_EVENTS];
	// �̺�Ʈ ������ ���� �̺�Ʈ ��ü
	WSAEVENT		eventHandle[WSA_MAXIMUM_WAIT_EVENTS];
	OverlappedEx	overlappedEx[WSA_MAXIMUM_WAIT_EVENTS];
};

class OverlappedEvent
{
private:
	// Ŭ���̾�Ʈ ���� ���� ����ü
	ClientInfo	m_clientInfo;
	// ���ӵǾ�	�ִ� Ŭ���̾�Ʈ ��
	int			m_nClientCnt;
	// ��Ŀ������ �ڵ�
	HANDLE		m_hWorkerThread;
	// ���ӽ����� �ڵ�
	HANDLE		m_hAccepterThread;
	// ��Ŀ������ ���� �÷���
	bool		m_bWorkerRun;
	// ���ӽ����� ���� �÷���
	bool		m_bAccepterRun;
	// ���� ����
	char		m_socketBuf[1024];

public:
	OverlappedEvent();
	~OverlappedEvent();

	//----���� Ŭ���̾�Ʈ ���� �Լ�----//
	// ���� �ʱ�ȭ
	bool InitSocket();
	// ���� ����
	void CloseSocket(SOCKET& sockClose, bool bIsForce = false);

	//----����----//
	bool BindAndListen(int bindPort);
	bool StartServer();

	// Overlapped I/O �۾��� ���� ������
	bool CreateWorkerThread();
	// accept ��û�� ó���ϴ� ������
	bool CreateAccepterThread();
	// ������ ���� index ��ȯ
	int GetEmptyIndex();
	// WSARecv Overlapped I/O �۾�
	bool RecvMsg(const int idx);
	// WSASend Overlapped I/O �۾�
	bool SendMsg(const int idx, const char* msg, const int len);

	// Overlapped I/O �۾� �Ϸ� �� �ش��ϴ� ó��
	void WorkerThread();
	// ������� ������ �޴� ������
	void AccepterThread();
	// Overlapped I/O �Ϸ� ó��
	void OverlappedResult(const int idx);
	// ������ �ı�
	void DestroyThread();

	bool Connect(const char* ip, unsigned short port);
	void ErrorHandling(const char* msg);
};