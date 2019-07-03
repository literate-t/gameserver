#include "IOCP.h"
#include "Protocol.h"

// WSARecv와 WSASend의 Overlapped I/O 작업 처리를 위한 스레드
unsigned int WINAPI CallWorkerThread(LPVOID p)
{
	IOCP* pIocp = (IOCP*)p;
	pIocp->WorkerThread();
	return 0;
}

// Client 접속을 위한 스레드
unsigned int WINAPI CallAccepterThread(LPVOID p)
{
	IOCP* pIocp = (IOCP*)p;
	pIocp->AccepterThread();
	return 0;
}

IOCP::IOCP()
{
	// 초기화
	m_bWorkerRun = true;
	m_bAccepterRun = true;
	m_nClientCnt = 0;
	m_hAccepterThread = NULL;
	m_hIOCP = NULL;
	m_socketListen = NULL;
	ZeroMemory(m_socketBuf, 1024);
	for (int i = 0; i < MAX_WORKERTHREAD; ++i)
		m_hWorkerThread[i] = NULL;
	//m_pClientInfo = new CLIENTINFO[MAX_CLIENT];
}

IOCP::~IOCP()
{
	// 윈속의 사용을 끝낸다
	WSACleanup();
	// 다 사용한 객체를 삭제
	//if (m_pClientInfo) {
	//	delete[] m_pClientInfo;
	//	m_pClientInfo = NULL;
	//}
}

bool IOCP::Init(const int maxClient)
{
	if (!InitSocket()) return false;
	auto poolSize = CreateClientPool(maxClient);
	printf("Client Pool Size:%d\n", poolSize);
	return true;
}

int IOCP::CreateClientPool(const int maxClient)
{
	for (int i = 0; i < maxClient; ++i)
	{
		CLIENTINFO client;
		client.index = i;
		m_clientPool.emplace_back(client);
		m_clientPoolIndex.emplace_back(client.index);
	}
	return maxClient;
}

bool IOCP::InitSocket()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR) {
		printf("WSAStartup() error");
		return false;
	}
	m_socketListen = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (m_socketListen == INVALID_SOCKET) {
		printf("WSASocket() error");
		return false;
	}
	printf("소켓 초기화 성공\n");
	return true;
}

void IOCP::CloseSocket(PCLIENTINFO& ClientInfo, bool bIsForce)
{
	linger lin = { 0, 0 };		// SO_DONTLINGER
	if (bIsForce == true)
		lin.l_onoff = 1;

	shutdown(ClientInfo->socket, SD_BOTH);
	setsockopt(ClientInfo->socket, SOL_SOCKET, SO_LINGER, (char*)& lin, sizeof(lin));
	closesocket(ClientInfo->socket);
	ClientInfo->socket = INVALID_SOCKET;
}

bool IOCP::BindAndListen(const int& port)
{
	SOCKADDR_IN			sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(port);
	sockAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(m_socketListen, (SOCKADDR*)& sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		printf("bind() error");
		return false;
	}
	if (listen(m_socketListen, 15) == SOCKET_ERROR) {
		printf("listen() error");
		return false;
	}
	printf("서버 등록 성공\n");
	return true;
}

bool IOCP::CreateWorkerThread()
{
	// WaitThread Queue에 넣을 스레드
	// 권장갯수: CPU 갯수 * 2 + 1
	for (int i = 0; i < MAX_WORKERTHREAD; ++i)
	{
		m_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, CallWorkerThread, this, CREATE_SUSPENDED, NULL);
		if (m_hWorkerThread[i] == NULL) {
			printf("CreateWorkerThread() error:%d", GetLastError());
			return false;
		}
		ResumeThread(m_hWorkerThread[i]);
	}
	printf("WorkerThread 시작\n");
	return true;
}

bool IOCP::CreateAccepterThread()
{
	// Client의 접속 요청을 받는 스레드
	m_hAccepterThread = (HANDLE)_beginthreadex(NULL, 0, CallAccepterThread, this, CREATE_SUSPENDED, NULL);
	if (m_hAccepterThread == NULL) {
		printf("[CreateAccepterThread()] CreateWorkerThread() error:%d", GetLastError());
		return false;
	}
	ResumeThread(m_hAccepterThread);
	printf("AccepterThread 시작\n");
	return true;
}

bool IOCP::BindIOCompletionPort(PCLIENTINFO& pClientInfo)
{
	if (pClientInfo == NULL)
		return false;
	HANDLE	hIOCP;
	// socket과 clientInfo를 CompletionPort 객체와 연결
	hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->socket, m_hIOCP, (ULONG_PTR)pClientInfo, 0);

	if (hIOCP == NULL) {
		printf("[BindIOCompletionPort] CreateIoCompletionPort() error:%d", GetLastError());
		return false;
	}
	return true;
}

bool IOCP::StartServer()
{
	// CompletionPort 객체 생성
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (m_hIOCP == NULL) {
		printf("CreateIoCompletionPort() error:%d", GetLastError());
		return false;
	}
	if (!CreateWorkerThread())	 return false;
	if (!CreateAccepterThread()) return false;
	printf("서버 시작\n");
	return true;
}

bool IOCP::RecvMsg(PCLIENTINFO& pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;
	int	  result;
	pClientInfo->recvOverlappedEx.wsaBuf.buf = pClientInfo->recvOverlappedEx.buf;
	pClientInfo->recvOverlappedEx.wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->recvOverlappedEx.op = OP_RECV;
	result = WSARecv(pClientInfo->socket, &pClientInfo->recvOverlappedEx.wsaBuf, 1,
		&dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED)& pClientInfo->recvOverlappedEx, NULL);
	// result가 SOCKET_ERROR이라면 클라이언트가 끊어진 것으로 처리
	if (result == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		printf("WSARecv() error");
		return false;
	}
	return true;
}

bool IOCP::SendMsg(PCLIENTINFO& pClientInfo, char* msg, const int len)
{
	DWORD dwRecvNumBytes = 0;
	// 전송할 메시지 복사
	CopyMemory(pClientInfo->sendOverlappedEx.buf, msg, len);

	pClientInfo->sendOverlappedEx.wsaBuf.len = len;
	pClientInfo->sendOverlappedEx.wsaBuf.buf = pClientInfo->sendOverlappedEx.buf;
	pClientInfo->sendOverlappedEx.op = OP_SEND;
	int result = WSASend(pClientInfo->socket, &pClientInfo->sendOverlappedEx.wsaBuf,
		1, &dwRecvNumBytes, 0, (LPWSAOVERLAPPED)& pClientInfo->sendOverlappedEx, NULL);
	// SOCKET_ERROR라면 client socket이 끊어진 걸로 처리
	if (result == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		printf("WSASend() error");
		return false;
	}
	return true;
}

bool IOCP::SendMsg(PCLIENTINFO& pClientInfo, const int len)
{
	DWORD dwRecvNumBytes = 0;
	// 전송할 메시지 복사
	CopyMemory(pClientInfo->sendOverlappedEx.buf, pClientInfo->recvBuffer, len);

	pClientInfo->sendOverlappedEx.wsaBuf.len = len;
	pClientInfo->sendOverlappedEx.wsaBuf.buf = pClientInfo->sendOverlappedEx.buf;
	pClientInfo->sendOverlappedEx.op = OP_SEND;
	int result = WSASend(pClientInfo->socket, &pClientInfo->sendOverlappedEx.wsaBuf,
		1, &dwRecvNumBytes, 0, (LPWSAOVERLAPPED)& pClientInfo->sendOverlappedEx, NULL);
	// SOCKET_ERROR라면 client socket이 끊어진 걸로 처리
	if (result == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		printf("WSASend() error");
		return false;
	}	
	return true;
}

int IOCP::GetEmptyClientInfo()
{
	if (m_clientPoolIndex.empty())
		return -1;
	auto index = m_clientPoolIndex.front();
	m_clientPoolIndex.pop_front();
	return index;
	//for (int i = 0; i < MAX_CLIENT; ++i)
	//{
	//	if (m_pClientInfo[i].socket == INVALID_SOCKET)
	//		return &m_pClientInfo[i];
	//}
	//return NULL;
}

// 사용자의 접속을 받는 스레드
void IOCP::AccepterThread()
{
	SOCKADDR_IN		sockAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	while (m_hAccepterThread)
	{
		// 접속을 받을 구조체의 인덱스
		auto index = GetEmptyClientInfo();
		if (index == -1) {
			printf("Client Full");
			return;
		}
		auto client = &m_clientPool[index];
		client->socket = accept(m_socketListen, (SOCKADDR*)& sockAddr, &addrLen);
		if (client->socket == NULL) continue;
		// IO CompletionPort 객체와 소켓을 연결
		if (!BindIOCompletionPort(client)) return;
		// Recv Overlapped IO 작업을 요청해놓는다
		if (!RecvMsg(client)) return;
		char msg[1024];
		printf("클라이언트 접속:IP(%s) SOCKET(%d)\n", inet_ntop(AF_INET, &sockAddr.sin_addr, msg, 1024), (int)client->socket);
		// 클라이언트 갯수 증가
		++m_nClientCnt;
	}
}

void IOCP::WorkerThread()
{
	// CompletionKey를 받을 포인터 변수
	PCLIENTINFO pClientInfo = NULL;
	// 함수호출 성공 여부
	BOOL result = TRUE;
	// Overlapped IO으로 전송된 데이터 크기
	DWORD dwIoSize = 0;
	// IO를 위해 요청한 Overlapped 구조체를 받을 포인터
	LPOVERLAPPED lpOverlapped = NULL;
	static int recvCount = 1;
	static int sendCount = 1;
	while (m_bWorkerRun)
	{
		// 이 함수로 인해 스레드는 WatingThread Queue에 대기 상태로 들어간다
		// Overlapped IO가 완료되면 IOCP Queue에서 가져와 처리
		// PostQueuedCompletionStatus()로 스레드 종료
		//result = GetQueuedCompletionStatus(m_hIOCP, &dwIoSize, (PULONG_PTR)& pClientInfo, &lpOverlapped, INFINITE);
		result = GetQueuedCompletionStatus(m_hIOCP, &dwIoSize, (PULONG_PTR)& pClientInfo, &lpOverlapped, INFINITE);
		if (result == TRUE && dwIoSize == 0) {
			printf("socket(%d) 접속 종료\n", (int)pClientInfo->socket);
			CloseSocket(pClientInfo);
			m_clientPool[pClientInfo->index].Clear();
			m_clientPoolIndex.emplace_back(pClientInfo->index);
			continue;
		}
		else if (result == TRUE && dwIoSize == 0 && lpOverlapped == NULL) {
			m_bWorkerRun = false;
			break;
		}
		else if (lpOverlapped == NULL) continue;

		OVERLAPPEDEX* pOverlappedEx = (OVERLAPPEDEX*)lpOverlapped;
		if (pOverlappedEx->op == OP_RECV)
		{
			pOverlappedEx->buf[dwIoSize] = NULL;
			if (!RecvProcess(pClientInfo, pOverlappedEx->buf, dwIoSize)) {
				printf("recv count:%d\n", recvCount++);
				continue;
			}
			printf("[수신][pOverlappedEx->buf] bytes:%d, msg:%s\n", dwIoSize, pClientInfo->recvBuffer);
			//SendMsg(pClientInfo, pOverlappedEx->buf, dwIoSize);
			//SendMsg(pClientInfo, dwIoSize);
			//RecvMsg(pClientInfo);
		}
		else if (pOverlappedEx->op == OP_SEND)
		{
			printf("send count:%d\n", sendCount++);
			printf("[송신] bytes:%d, msg:%s\n", pClientInfo->totalSize, pOverlappedEx->buf);
			ZeroMemory(pOverlappedEx->buf, sizeof(pOverlappedEx->buf));
		}
		// 예외
		else
			printf("socket(%d)에서 예외 발생\n", (int)pClientInfo->socket);
	}
}

bool IOCP::RecvProcess(PCLIENTINFO& client, char* msg, size_t receivedLen)
{
	PACKET* packet	= NULL;
	char*   m		= NULL;
	int		len		= 0;
	// 패킷을 최초에 받을 때
	if (client->remainingDataSize == 0) {
		packet = (PACKET*)msg;
		m = packet->msg;		
		client->remainingDataSize = packet->length - PACKET_HEADER_SIZE;
		client->totalSize = packet->length - PACKET_HEADER_SIZE;
		client->readPos = 0;
		len = receivedLen - PACKET_HEADER_SIZE;
	}
	// 처음 이후
	else {
		m = msg;
		len = receivedLen;
	}
	CopyMemory(&client->recvBuffer[client->readPos], m, len);
	client->readPos += len;
	client->remainingDataSize -= len;
	// 데이터 전부 수신
	if (client->remainingDataSize == 0) {
		client->readPos = 0;		
		SendMsg(client, client->totalSize);
		RecvMsg(client);
		return true;
	}
	RecvMsg(client);
	return false;
}

void IOCP::DestroyThread()
{
	for (int i = 0; i < MAX_WORKERTHREAD; ++i)
	{
		// WatingThread Queue에서 대기 중인 스레드에 종료 메시지 보내기
		PostQueuedCompletionStatus(m_hIOCP, 0, 0, NULL);
	}
	for (int i = 0; i < MAX_WORKERTHREAD; ++i)
	{
		CloseHandle(m_hWorkerThread[i]);
		WaitForSingleObject(m_hWorkerThread[i], INFINITE);
		//m_hWorkerThread[i] = NULL;
	}
	m_bAccepterRun = false;
	CloseHandle(m_hAccepterThread);
	WaitForSingleObject(m_hAccepterThread, INFINITE);
	closesocket(m_socketListen);
}