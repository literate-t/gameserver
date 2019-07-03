#include "Precompile.h"

Connection::Connection()
{
	m_sockListener			= INVALID_SOCKET;
	m_socket				= INVALID_SOCKET;
	m_lpRecvOverlappedEx	= NULL;
	m_lpSendOverlappedEx	= NULL;
	m_nSendBuffSize			= 0;
	m_nRecvBuffSize			= 0;
	InitializeConnection();
}

void Connection::InitializeConnection()
{
	ZeroMemory(m_Ip, MAX_IP_LENGTH);
	m_socket		= INVALID_SOCKET;
	m_bIsConnected = false;
	m_bIsClosed = false;
	m_bIsSent = true;
	m_dwSendIoRefCount		= 0;
	m_dwRecvIoRefCount		= 0;
	m_dwAcceptIoRefCount	= 0;

	m_ringRecvBuffer.Initialize();
	m_ringSendBuffer.Initialize();
}

Connection::~Connection(void)
{
	m_sockListener = INVALID_SOCKET;
	m_socket = INVALID_SOCKET;
}

bool Connection::CreateConnection(INITCONFIG& initConfig)
{
	m_nIndex = initConfig.nIndex;
	m_sockListener = initConfig.sockListener;

	m_lpRecvOverlappedEx = new OVERLAPPED_EX(this);
	m_lpSendOverlappedEx = new OVERLAPPED_EX(this);
	m_ringRecvBuffer.Create(initConfig.nRecvBufSize * initConfig.nRecvBufCnt);
	m_ringSendBuffer.Create(initConfig.nSendBufSize * initConfig.nSendBufCnt);
	m_nRecvBuffSize = initConfig.nRecvBufSize;
	m_nSendBuffSize = initConfig.nSendBufSize;

	return BindAcceptExSock();
}

bool Connection::BindAcceptExSock()
{
	DWORD	dwBytes;
	ZeroMemory(&m_lpRecvOverlappedEx->s_Overlapped, sizeof(OVERLAPPED));
	m_lpRecvOverlappedEx->s_WsaBuf.buf	= m_addressBuff;
	m_lpRecvOverlappedEx->s_lpSocketMsg = &m_lpRecvOverlappedEx->s_WsaBuf.buf[0];
	m_lpRecvOverlappedEx->s_WsaBuf.len = m_nRecvBuffSize;
	m_lpRecvOverlappedEx->s_eOperation = OP_ACCEPT;
	m_lpRecvOverlappedEx->s_lpConnection = this;
	m_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET) {
		LOG(LOG_ERROR_NORMAL, "Connection::BindAcceptExSock() | WSASocket() Faild():error[%u]\n", GetLastError());
		return false;
	}
	IncrementAcceptIoRefCount();
	bool result = AcceptEx(m_sockListener, m_socket, m_lpRecvOverlappedEx->s_WsaBuf.buf, 0, 
		sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes, (LPOVERLAPPED)m_lpRecvOverlappedEx);
	if (!result && WSAGetLastError() != WSA_IO_PENDING)
	{
		LOG(LOG_ERROR_NORMAL, "Connection::BindAcceptExSock() | AcceptEx() Faild():error[%u]\n", GetLastError());
		return false;
	}
	return true;
}

bool Connection::CloseConnection(bool bForce)
{
	Monitor::Owner lock(m_csConnection);
	linger lin = { 0, 0 }; // SO_DONTLINGER
	if (bForce == true)
		lin.l_onoff = 1; // SO_LINGER, timeout = 0
	if (IocpServer() != NULL && m_bIsConnected == true)
		IocpServer()->CloseConnection(this);
	shutdown(m_socket, SD_BOTH);
	setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)& lin, sizeof(lin));
	closesocket(m_socket);

	m_socket = INVALID_SOCKET;
	if (m_lpRecvOverlappedEx != NULL) {
		m_lpRecvOverlappedEx->s_dwRemain = 0;
		m_lpRecvOverlappedEx->s_nTotalBytes = 0;
	}
	if (m_lpSendOverlappedEx != NULL) {
		m_lpSendOverlappedEx->s_dwRemain = 0;
		m_lpSendOverlappedEx->s_nTotalBytes = 0;
	}
	// connection 다시 초기화
	InitializeConnection();
	BindAcceptExSock();

	return true;
}

char* Connection::PrepareSendPacket(const int len)
{
	if (m_bIsConnected == false)
		return NULL;
	char* buf = m_ringSendBuffer.ForwardMark(len);
	if (buf == NULL)
	{
		IocpServer()->CloseConnection(this);
		LOG(LOG_ERROR_NORMAL, "Connection::PrepareSendPacket() | socket[%d] SendRingBuffer overflow\n", m_socket);
		return NULL;
	}
	ZeroMemory(buf, len);
	CopyMemory(buf, &len, PACKET_SIZE_LENGTH);
	return buf;
}

bool Connection::ReleaseSendPacket(LPOVERLAPPED_EX lpSendOvlppedEx)
{
	if (lpSendOvlppedEx == NULL)
		return false;
	m_ringSendBuffer.ReleaseBuffer(m_lpSendOverlappedEx->s_WsaBuf.len);
	return true;
}

bool Connection::BindIOCP(HANDLE& hIOCP)
{
	HANDLE	hIOCPHandle;
	Monitor::Owner lock(m_csConnection);

	hIOCPHandle = CreateIoCompletionPort((HANDLE)m_socket, hIOCP, (DWORD)this, 0);
	if (hIOCPHandle == NULL) {
		LOG(LOG_ERROR_NORMAL, "Connection::BindIOCP() | CreateIoCompletionPort() Faild\n", GetLastError());
		return false;
	}
	m_hIOCP = hIOCP;
	return true;
}

bool Connection::RecvPost(const char* next, DWORD dwRemain)
{
	int		result = 0;
	DWORD	dwFlag = 0;
	DWORD	dwRecvNumBytes = 0;

	if (m_bIsConnected == false || m_lpRecvOverlappedEx == NULL)
		return false;
	m_lpRecvOverlappedEx->s_eOperation = OP_RECV;
	m_lpRecvOverlappedEx->s_dwRemain = dwRemain;	
	int moveMark = dwRemain - (m_ringRecvBuffer.GetCurrentMark() - next); // ?
	m_lpRecvOverlappedEx->s_WsaBuf.buf = m_ringRecvBuffer.ForwardMark(moveMark, m_nRecvBuffSize, dwRemain);

	if (m_lpRecvOverlappedEx->s_WsaBuf.buf == NULL) {
		IocpServer()->CloseConnection(this);
		LOG(LOG_ERROR_NORMAL, "Connection::RecvPost() | Socket[%d] RecvRingBuffer RecvRingBuffer\n", m_socket);
		return false;
	}
	m_lpRecvOverlappedEx->s_lpSocketMsg = m_lpRecvOverlappedEx->s_WsaBuf.buf - dwRemain;
	memset(&m_lpRecvOverlappedEx->s_Overlapped, 0, sizeof(OVERLAPPED));
	IncrementRecvIoRefCount();
	result = WSARecv(m_socket, &m_lpRecvOverlappedEx->s_WsaBuf, 1, &dwRecvNumBytes, &dwFlag, &m_lpRecvOverlappedEx->s_Overlapped, NULL);
	if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
		DecrementRecvIoRefCount();
		IocpServer()->CloseConnection(this);
		LOG(LOG_ERROR_NORMAL, "Connection::RecvPost() | WSARecv() Failed: %u\n", GetLastError());
		return false;
	}
	return true;
}

bool Connection::SendPost(int sendSize)
{
	DWORD dwBytes;
	//보내는 양이 있다면, 즉 IocpServer class의 DoSend()에서 불린게 아니라 PrepareSendPacket()함수를 부르고 
	//SendPost가 불렸다면 보내는 양이 있고 DoSend에서 불렸다면 0이 온다.
	if (sendSize > 0)
		m_ringSendBuffer.SetUsedBufferSize(sendSize);
	if (InterlockedCompareExchange((LPLONG)& m_bIsSent, FALSE, TRUE) == TRUE)
	{
		int readSize = 0;
		char* buf = m_ringSendBuffer.GetBuffer(m_nSendBuffSize, &readSize);
		if (buf == NULL) {
			InterlockedExchange((LPLONG)m_bIsSent, TRUE);
			return false;
		}
		m_lpSendOverlappedEx->s_dwRemain = 0;
		m_lpSendOverlappedEx->s_eOperation = OP_SEND;
		m_lpSendOverlappedEx->s_nTotalBytes = readSize;
		ZeroMemory(&m_lpSendOverlappedEx->s_Overlapped, sizeof(OVERLAPPED));
		m_lpSendOverlappedEx->s_WsaBuf.len = readSize;
		m_lpSendOverlappedEx->s_WsaBuf.buf = buf;
		m_lpSendOverlappedEx->s_lpConnection = this;
		
		IncrementSendIoRefCount();
		int result = WSASend(m_socket, &m_lpSendOverlappedEx->s_WsaBuf, 1, &dwBytes, 0, &m_lpSendOverlappedEx->s_Overlapped, NULL);
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			DecrementSendIoRefCount();
			IocpServer()->CloseConnection(this);
			LOG(LOG_ERROR_NORMAL, "Connection::SendPost() socket[%u] | WSASend(): SOCKET_ERROR, %u\n", m_socket, GetLastError());
			InterlockedExchange((LPLONG)& m_bIsSent, FALSE);
			return false;
		}
	}
	return true;
}