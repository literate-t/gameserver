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
	// 클라이언트 정보 저장 
	std::deque<CLIENTINFO>		m_clientPool;
	std::deque<int>				m_clientPoolIndex;
	// 클라이언트 접속을 받기 위한 리슨 소켓
	SOCKET				m_socketListen;
	// 접속되어 있는 클라이언트 수
	int					m_nClientCnt;
	// 작업 스레드 핸들(WaitingThread Queue에 들어갈 스레드)
	HANDLE				m_hWorkerThread[MAX_WORKERTHREAD];
	// 접속 스레드 핸들
	HANDLE				m_hAccepterThread;
	// 패킷 스레드 핸들
	HANDLE				m_hPktProcessThread;
	// CompletionPort 객체 핸들
	HANDLE				m_hIOCP;
	// 작업 스레드 동작 플래그
	bool				m_bWorkerRun;
	// 접속 스레드 동작 플래그
	bool				m_bAccepterRun;
	// 패킷 스래드 동작 플래그
	bool				m_bPktProcessRun;
	// 소켓 버퍼
	char				m_socketBuf[1024];
	// 패킷 큐
	std::deque<PACKET>	m_packetQueue;
	// 패킷 프로세스 클래스
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
	// WaitingThread Queue에서 대기할 스레드 생성
	bool CreateWorkerThread();
	// Accept 요청을 처리할 스레드 생성
	bool CreateAccepterThread();
	// 큐에 담긴 패킷을 처리하는 스레드 생성
	bool CreatePktProcessThread();
	// 사용하지 않는 클라이언트 정보 구조체를 반환
	int GetEmptyClientInfo();
	// CompletionPort 객체, 소켓, CompletionKey를 연결
	bool BindIOCompletionPort(PCLIENTINFO& clientInfo);
	// 패킷 처리
	bool RecvProcess(PCLIENTINFO& clienfInfo, char* msg, size_t msgLength);
	// WSARecv Overlapped I/O 작업
	bool RecvMsg(PCLIENTINFO& pClientInfo);
	// WSASend Overlapped I/O 작업
	bool SendMsg(PCLIENTINFO& pClientInfo, char* msg, const int len);
	bool SendMsg(PCLIENTINFO& pClientInfo, const int len);
	// 데이터 보내기
	void SendData();
	// 완료된 Overlapped I/O 처리 스레드
	void WorkerThread();
	// 사용자 접속 받는 스레드
	void AccepterThread();
	// 큐에 담긴 패킷을 처리하는 스레드
	void PktProcessThread();
	// 큐에 패킷을 담기만 하는 함수
	void AddPacketQueue(PACKET& packet);
	PACKET GetPacket();
	void DestroyThread();
};