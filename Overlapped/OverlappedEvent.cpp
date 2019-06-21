#include "OverlappedEvent.h"

unsigned int WINAPI CallWorkerThread(LPVOID p)
{
	OverlappedEvent* pOverlappedEvent = reinterpret_cast<OverlappedEvent*>(p);
	pOverlappedEvent->WorkerThread();
	return 0;
}

unsigned int WINAPI CallAccepterThread(LPVOID p)
{
	OverlappedEvent* pOverlappedEvent = reinterpret_cast<OverlappedEvent*>(p);
	pOverlappedEvent->AccepterThread();
	return 0;
}

OverlappedEvent::OverlappedEvent()
{
	m_bWorkerRun		= true;
	m_bAccepterRun		= true;
	m_nClientCnt		= 0;
	m_hWorkerThread		= NULL;
	m_hAccepterThread	= NULL;
	ZeroMemory(m_socketBuf, 1024);

	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; ++i)
	{
		m_clientInfo.sockClient[i] = INVALID_SOCKET;
		m_clientInfo.eventHandle[i] = WSACreateEvent(); // non-signal, manual-mode
		ZeroMemory(&m_clientInfo.overlappedEx[i], sizeof(WSAOVERLAPPED));
	}
}

OverlappedEvent::~OverlappedEvent()
{
	WSACleanup();
	
	closesocket(m_clientInfo.sockClient[0]);
	SetEvent(m_clientInfo.eventHandle[0]);
	m_bWorkerRun	= false;
	m_bAccepterRun	= false;
	WaitForSingleObject(m_hWorkerThread, INFINITE);
	WaitForSingleObject(m_hAccepterThread, INFINITE);
}

void OverlappedEvent::ErrorHandling(const char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

// 소켓 초기화
bool OverlappedEvent::InitSocket()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup() Error");
		return false;
	}

	// TCP, Overlapped I/O 소켓 생성
	m_clientInfo.sockClient[0] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (m_clientInfo.sockClient[0] == INVALID_SOCKET) {
		ErrorHandling("WSASocket() error");
		return false;
	}
	return true;
}

void OverlappedEvent::CloseSocket(SOCKET& socketClose, bool bIsForce)
{
	// SO_DONTLINGER 설정
	linger lin = { 0, 0 };

	// bISForce가 true면 SO_LINGER, timeout = 0으로 설정하여 강제 종료
	if (bIsForce == true)
		lin.l_onoff = 1;
	// sockClose의 데이터 송수신 모두 중단
	shutdown(socketClose, SD_BOTH);
	// 소켓 옵션 설정
	setsockopt(socketClose, SOL_SOCKET, SO_LINGER,
		reinterpret_cast<char*>(&lin), sizeof(lin));
	// 소켓 연결 종료
	closesocket(socketClose);
	socketClose = INVALID_SOCKET;
}

bool OverlappedEvent::BindAndListen(int port)
{
	SOCKADDR_IN		sockAddr;
	ZeroMemory(&sockAddr, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	sockAddr.sin_port = htons(port);

	if (bind(m_clientInfo.sockClient[0],
		(sockaddr*)& sockAddr, sizeof(sockAddr)) == SOCKET_ERROR) {
		ErrorHandling("bind() error");
		return false;
	}

	if (listen(m_clientInfo.sockClient[0], 15) == SOCKET_ERROR) {
		ErrorHandling("listen() error");
		return false;
	}
	//std::cout << "서버 접속 대기 중" << std::endl;
	//std::cout << "BindAndListen() m_bWorkerRun " << m_bWorkerRun << std::endl;
	return true;
}

bool OverlappedEvent::CreateWorkerThread()
{
	m_hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, &CallWorkerThread,
		this, CREATE_SUSPENDED, NULL);
	if (m_hWorkerThread == NULL) {
		ErrorHandling("WorkerThread() _beginthreadex() error");
		return false;
	}
	ResumeThread(m_hWorkerThread);
	std::cout << "WorkerThread 시작" << std::endl;
	return true;
}

bool OverlappedEvent::CreateAccepterThread()
{
	m_hAccepterThread = (HANDLE)_beginthreadex(NULL, 0, &CallAccepterThread,
		this, CREATE_SUSPENDED, NULL);
	if (m_hAccepterThread == NULL) {
		ErrorHandling("AccepterThread() _beginthreadex() error");
		return false;
	}
	ResumeThread(m_hAccepterThread);
	std::cout << "AccepterThread 시작" << std::endl;
	return true;
}

// 사용되지 않은 index 반환
int OverlappedEvent::GetEmptyIndex()
{
	// 0번째 배열은 정보갱신용 이벤트
	for (int i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; ++i)
	{
		if (m_clientInfo.sockClient[i] == INVALID_SOCKET)
			return i;
	}
	return -1;
}

bool OverlappedEvent::StartServer()
{
	// 접속된 클라이언트 주소 정보를 저장할 구조체
	if (!CreateWorkerThread())
		return false;
	if (!CreateAccepterThread())
		return false;
	// 정보 갱신용 이벤트 생성
	m_clientInfo.eventHandle[0] = WSACreateEvent(); // non-signal, manual reset mode
	std::cout << "서버 시작" << std::endl;
	return true;
}

bool OverlappedEvent::RecvMsg(const int idx)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;
	m_clientInfo.eventHandle[idx] = WSACreateEvent();

	// Overlapped I/O를 위한 설정
	m_clientInfo.overlappedEx[idx].wsaOverlapped.hEvent = m_clientInfo.eventHandle[idx];
	m_clientInfo.overlappedEx[idx].wsaBuf.len = MAX_SOCKBUF;
	m_clientInfo.overlappedEx[idx].wsaBuf.buf = m_clientInfo.overlappedEx[idx].buf;
	m_clientInfo.overlappedEx[idx].idx = idx;
	m_clientInfo.overlappedEx[idx].op = OP_RECV;

	int result = WSARecv(m_clientInfo.sockClient[idx], &(m_clientInfo.overlappedEx[idx].wsaBuf),
		1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED)&m_clientInfo.overlappedEx[idx], NULL);
	if (result == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		ErrorHandling("WSARecv() error");
		return false;
	}
}

bool OverlappedEvent::SendMsg(const int idx, const char* msg, const int len)
{
	DWORD dwSentNumBytes = 0;

	// 전송할 메시지를 복사
	CopyMemory(m_clientInfo.overlappedEx[idx].buf, msg, len);

	// Overlapped I/O를 위한 설정
	m_clientInfo.overlappedEx[idx].wsaOverlapped.hEvent = m_clientInfo.eventHandle[idx];
	m_clientInfo.overlappedEx[idx].wsaBuf.len = len;
	m_clientInfo.overlappedEx[idx].wsaBuf.buf = m_clientInfo.overlappedEx[idx].buf;
	m_clientInfo.overlappedEx[idx].idx = idx;
	m_clientInfo.overlappedEx[idx].op = OP_SEND;

	int result = WSASend(m_clientInfo.sockClient[idx], &m_clientInfo.overlappedEx[idx].wsaBuf,
		1, &dwSentNumBytes, 0, (LPWSAOVERLAPPED)& m_clientInfo.overlappedEx[idx], NULL);
	if (result == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		ErrorHandling("WSASend() error");
		return false;
	}
	return true;
}

void OverlappedEvent::AccepterThread()
{
	SOCKADDR_IN		clientAddr;
	int addrLen = sizeof(clientAddr);
	bool result;
	while (m_bAccepterRun)
	{
		// 접속받을 구조체의 인덱스
		int idx = GetEmptyIndex();
		if (idx == -1) {
			ErrorHandling("Client Full");
			return;
		}
		// 클라이언트가 접속 요청할 때까지 대기
		m_clientInfo.sockClient[idx] = accept(m_clientInfo.sockClient[0],
			(SOCKADDR*)& clientAddr, &addrLen);
		if (m_clientInfo.sockClient[idx] == INVALID_SOCKET)
			return;
		result = RecvMsg(idx);
		if (result == false)
			return;
		printf("클라이언트 접속: IP(%s), SOCKET(%d)\n",
			inet_ntoa(clientAddr.sin_addr), m_clientInfo.sockClient[idx]);
		// 클라이언트 수 증가
		++m_nClientCnt;
		// 클라이언트가 접속되었기 때문에 WorkerThread로 정보 갱신 알림
		WSASetEvent(m_clientInfo.eventHandle[0]); // to signal-state
	}
}

void OverlappedEvent::WorkerThread()
{
	//std::cout << "WorkerThread() m_bWorkerRun "<< m_bWorkerRun << std::endl;
	while (m_bWorkerRun)
	{
		//std::cout << "WorkerThread() start" << std::endl;
		// 요청된 Overlapped I/O작업 이벤트 대기
		// m_clientInfo.eventHandle[0] 때문에 처음 반환되는 idx는 0일 것으로 예상
		DWORD dwObjIdx = WSAWaitForMultipleEvents(WSA_MAXIMUM_WAIT_EVENTS, m_clientInfo.eventHandle,
			FALSE, INFINITE, FALSE);
		// 에러
		if (dwObjIdx == WSA_WAIT_FAILED) {
			ErrorHandling("WSAWaitForMultipleEvents() error");
			break;
		}
		// 이벤트 리셋
		WSAResetEvent(m_clientInfo.eventHandle[dwObjIdx]);
		// 접속이 들어왔다(?)
		if (dwObjIdx == WSA_WAIT_EVENT_0) continue;
		// Overlapped I/O 처리
		OverlappedResult(dwObjIdx);
	}
	//std::cout << "WorkerThread() end" << std::endl;
}

void OverlappedEvent::OverlappedResult(const int idx)
{
	DWORD dwTransfer = 0;
	DWORD dwFlags	= 0;
	bool result = WSAGetOverlappedResult(m_clientInfo.sockClient[idx],
		(LPWSAOVERLAPPED)& m_clientInfo.overlappedEx[idx], &dwTransfer, FALSE, &dwFlags);
	if (result == TRUE && dwTransfer == 0) {
		ErrorHandling("WSAGetOverlappedResult() error");
		return;
	}

	// 접속 종료
	if (dwTransfer == 0) {
		printf("접속 종료:socket:%d\n", m_clientInfo.sockClient[idx]);
		CloseSocket(m_clientInfo.sockClient[idx]);
		--m_nClientCnt;
		return;
	}

	OverlappedEx* pOverlappedEx = &m_clientInfo.overlappedEx[idx];
	switch (pOverlappedEx->op)
	{
		// WSARecv로 Overlapped I/O 완료
		case OP_RECV:
		{
			pOverlappedEx->buf[dwTransfer] = NULL;
			printf("[수신] bytes: %d, msg: %s\n", dwTransfer, pOverlappedEx->buf);
			// 에코
			SendMsg(idx, pOverlappedEx->buf, dwTransfer);
			break;
		}

		// WSASend로 Overlapped I/O 완료
		case OP_SEND:
		{
			pOverlappedEx->buf[dwTransfer] = NULL;
			printf("[송신] bytes: %d, msg: %s\n", dwTransfer, pOverlappedEx->buf);
			// 다시 Recv Operation I/O를 건다
			RecvMsg(idx);
			break;
		}
		default:
		{
			std::cout << "정의되지 않은 작업입니다\n" << std::endl;
			break;
		}
	}
}

void OverlappedEvent::DestroyThread()
{
	closesocket(m_clientInfo.sockClient[0]);
	SetEvent(m_clientInfo.eventHandle[0]);
	m_bWorkerRun = false;
	m_bAccepterRun = false;
	WaitForSingleObject(m_hWorkerThread, INFINITE);
	WaitForSingleObject(m_hAccepterThread, INFINITE);
}

bool OverlappedEvent::Connect(const char* ip, unsigned short port) {
	SOCKADDR_IN		servAddr;
	char			msg[MAX_SOCKBUF];
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.S_un.S_addr = inet_addr(ip);
	servAddr.sin_port = htons(port);
	if (connect(m_clientInfo.sockClient[0], reinterpret_cast<SOCKADDR*>(&servAddr), 
		sizeof(servAddr)) == SOCKET_ERROR) {
		//ErrorHandling("connect() error");
		std::cout << "connect() error" << std::endl;
		std::cout << "ErrorCode:" << WSAGetLastError() << std::endl;
		return false;
	}
	std::cout << "접속 성공" << std::endl;
	while (true) {
		std::cout << ">>";
		std::cin >> msg;
		if (!(_strcmpi(msg, "q") || _strcmpi(msg, "Q"))) break;

		int sendLen = send(m_clientInfo.sockClient[0], msg, strlen(msg), 0);
		if (sendLen == -1) {
			ErrorHandling("Socket::Connect() send() fail");
			std::cout << "ErrorCode:" << WSAGetLastError() << std::endl;
			return false;
		}
		std::cout << "메시지송신: 송신bytes[" << sendLen << "], 내용:[" << msg << "]" << std::endl;

		int recvLen = recv(m_clientInfo.sockClient[0], m_socketBuf, MAX_SOCKBUF, 0);
		if (recvLen == 0) {
			std::cout << "클라이언트와의 연결이 종료되었습니다" << std::endl;
			CloseSocket(m_clientInfo.sockClient[0]);
			return false;
		}
		else if (recvLen == -1) {
			ErrorHandling("Connect() recv() fail");
			std::cout << "ErrorCode:" << WSAGetLastError() << std::endl;
			return false;
		}
		m_socketBuf[recvLen] = NULL;
		std::cout << "메시지수신: 수신bytes[" << recvLen << "], 내용:[" << m_socketBuf << "]" << std::endl;
	}
	CloseSocket(m_clientInfo.sockClient[0]);
	std::cout << "클라이언트 정상 종료" << std::endl;
	return true;
}