#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <process.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <deque>
#include "Protocol.h"
#include "PacketProcess.h"

#define MAX_SOCKBUF			1024 * 10
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
	char			buf[1024];
	eOPERATION		op;
};

typedef struct _CLIENTINFO {
	SOCKET			socket;
	OVERLAPPEDEX	recvOverlappedEx;
	OVERLAPPEDEX	sendOverlappedEx;
	int				index;
	size_t			remainingDataSize;
	size_t			totalSize;
	size_t			readPos;
	char			recvBuffer[MAX_SOCKBUF];
	char			sendBuffer[MAX_SOCKBUF];
	short			packetId;
	_CLIENTINFO()
	{
		ZeroMemory(&recvOverlappedEx, sizeof(OVERLAPPEDEX));
		ZeroMemory(&sendOverlappedEx, sizeof(OVERLAPPEDEX));
		socket = INVALID_SOCKET;
		index = 0;
		remainingDataSize = 0;
		readPos = 0;
		totalSize = 0;
		ZeroMemory(recvBuffer, sizeof(recvBuffer));
		ZeroMemory(sendBuffer, sizeof(recvBuffer));
	}
	bool IsConnected() { socket != INVALID_SOCKET ? true : false; }
	void Clear()
	{
		ZeroMemory(&recvOverlappedEx, sizeof(OVERLAPPEDEX));
		ZeroMemory(&sendOverlappedEx, sizeof(OVERLAPPEDEX));
		socket = INVALID_SOCKET;
		index = 0;
		remainingDataSize = 0;
		readPos = 0;
		totalSize = 0;
		ZeroMemory(recvBuffer, sizeof(recvBuffer));
		ZeroMemory(sendBuffer, sizeof(recvBuffer));
	}
	
}CLIENTINFO, *PCLIENTINFO;
class IOCP
{
private:
	// Ŭ���̾�Ʈ ���� ���� 
	std::deque<CLIENTINFO>		m_clientPool;
	std::deque<int>				m_clientPoolIndex;
	// Ŭ���̾�Ʈ ������ �ޱ� ���� ���� ����
	SOCKET				m_socketListen;
	// ���ӵǾ� �ִ� Ŭ���̾�Ʈ ��
	int					m_nClientCnt;
	// �۾� ������ �ڵ�(WaitingThread Queue�� �� ������)
	HANDLE				m_hWorkerThread[MAX_WORKERTHREAD];
	// ���� ������ �ڵ�
	HANDLE				m_hAccepterThread;
	// ��Ŷ ������ �ڵ�
	HANDLE				m_hPktProcessThread;
	// CompletionPort ��ü �ڵ�
	HANDLE				m_hIOCP;
	// �۾� ������ ���� �÷���
	bool				m_bWorkerRun;
	// ���� ������ ���� �÷���
	bool				m_bAccepterRun;
	// ��Ŷ ������ ���� �÷���
	bool				m_bPktProcessRun;
	// ���� ����
	char				m_socketBuf[1024];
	// ��Ŷ ť
	std::deque<PACKET>	m_packetQueue;
	// ��Ŷ ���μ��� Ŭ����
	PacketProcess		m_packetProcess;

public:
	IOCP();
	~IOCP();

	bool Init(const int maxClient);
	void Release();
	bool InitSocket();
	int CreateClientPool(const int maxClient);
	void CloseSocket(PCLIENTINFO& ClientInfo, bool bIsForce = false);
	
	bool BindAndListen(const int& port);
	bool StartServer();
	// WaitingThread Queue���� ����� ������ ����
	bool CreateWorkerThread();
	// Accept ��û�� ó���� ������ ����
	bool CreateAccepterThread();
	// ť�� ��� ��Ŷ�� ó���ϴ� ������ ����
	bool CreatePktProcessThread();
	// ������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ
	int GetEmptyClientInfo();
	// CompletionPort ��ü, ����, CompletionKey�� ����
	bool BindIOCompletionPort(PCLIENTINFO& clientInfo);
	// ��Ŷ ó��
	bool RecvProcess(PCLIENTINFO& clienfInfo, char* msg, size_t msgLength);
	// WSARecv Overlapped I/O �۾�
	bool RecvMsg(PCLIENTINFO& pClientInfo);
	// WSASend Overlapped I/O �۾�
	bool SendMsg(PCLIENTINFO& pClientInfo, char* msg, const int len);
	bool SendMsg(PCLIENTINFO& pClientInfo, const int len);
	// ������ ������
	void SendData();
	// �Ϸ�� Overlapped I/O ó�� ������
	void WorkerThread();
	// ����� ���� �޴� ������
	void AccepterThread();
	// ť�� ��� ��Ŷ�� ó���ϴ� ������
	void PktProcessThread();
	// ť�� ��Ŷ�� ��⸸ �ϴ� �Լ�
	void AddPacketQueue(PACKET& packet);
	PACKET GetPacket();
	void DestroyThread();
};