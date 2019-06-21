#pragma once
#include "Precompile.h"
#define MAX_SOCKBUF 1024

enum eOperation {
	OP_RECV,
	OP_SEND
};

struct OverlappedEx {
	WSAOVERLAPPED	wsaOverlapped;
	int				idx;	// 구조체 배열 인덱스
	WSABUF			wsaBuf;	// Overlapped I/O 작업 버퍼
	char			buf[MAX_SOCKBUF];
	eOperation		op;		// 동작 종류
};

// 클라이언트 정보를 담기 위한 구조체
// 0번째 배열에 더미로(?) 접속이 들어오면 새로운 접속의 이벤트 감지를 위해
// WSAWaitForMultipleEvents를 다시 걸어준다
struct ClientInfo {
	// Client와 연결되는 소켓
	SOCKET			sockClient[WSA_MAXIMUM_WAIT_EVENTS];
	// 이벤트 감지를 위한 이벤트 객체
	WSAEVENT		eventHandle[WSA_MAXIMUM_WAIT_EVENTS];
	OverlappedEx	overlappedEx[WSA_MAXIMUM_WAIT_EVENTS];
};

class OverlappedEvent
{
private:
	// 클라이언트 정보 저장 구조체
	ClientInfo	m_clientInfo;
	// 접속되어	있는 클라이언트 수
	int			m_nClientCnt;
	// 워커스레드 핸들
	HANDLE		m_hWorkerThread;
	// 접속스레드 핸들
	HANDLE		m_hAccepterThread;
	// 워커스레드 동작 플래그
	bool		m_bWorkerRun;
	// 접속스레드 동작 플래그
	bool		m_bAccepterRun;
	// 소켓 버퍼
	char		m_socketBuf[1024];

public:
	OverlappedEvent();
	~OverlappedEvent();

	//----서버 클라이언트 공통 함수----//
	// 소켓 초기화
	bool InitSocket();
	// 소켓 종료
	void CloseSocket(SOCKET& sockClose, bool bIsForce = false);

	//----서버----//
	bool BindAndListen(int bindPort);
	bool StartServer();

	// Overlapped I/O 작업을 위한 스레스
	bool CreateWorkerThread();
	// accept 요청을 처리하는 스레드
	bool CreateAccepterThread();
	// 사용되지 않은 index 반환
	int GetEmptyIndex();
	// WSARecv Overlapped I/O 작업
	bool RecvMsg(const int idx);
	// WSASend Overlapped I/O 작업
	bool SendMsg(const int idx, const char* msg, const int len);

	// Overlapped I/O 작업 완료 후 해당하는 처리
	void WorkerThread();
	// 사용자의 접속을 받는 스레드
	void AccepterThread();
	// Overlapped I/O 완료 처리
	void OverlappedResult(const int idx);
	// 스레드 파괴
	void DestroyThread();

	bool Connect(const char* ip, unsigned short port);
	void ErrorHandling(const char* msg);
};