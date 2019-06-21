#include "Precompile.h"

Connection::Connection(void)
{
	m_sockListener			= INVALID_SOCKET;
	m_socket				= INVALID_SOCKET;
	m_lpRecvOverlappedEx	= NULL;
	m_lpSendOverlappedEx	= NULL;

	m_nSendBufferSize			= 0;
	m_nRecvBufferSize			= 0;

	InitializeConnection();
}

void Connection::InitializeConnection()
{
	ZeroMemory(m_szIp, MAX_IP_LENGTH);
	m_socket				= INVALID_SOCKET;
	m_bIsConnect			= FALSE;
	m_bIsClosed				= FALSE;
	m_bIsSend				= TRUE;
	m_dwSendIoRefCount		= 0;
	m_dwRecvIoRefCount		= 0;
	m_dwAcceptIoRefCount	= 0;
	m_ringRecvBuffer.Initialize();
	m_ringSendBuffer.Initialize();
}

Connection::~Connection(void)
{
	m_sockListener	= INVALID_SOCKET;
	m_socket		= INVALID_SOCKET;
}

bool Connection::CreateConnection(INITCONFIG& initConfig)
{
	m_nIndex		= initConfig.nIndex;
	m_sockListener	= initConfig.sockListener;		// SOCKADDR_IN 변수를 통해 IP, PORT가 바인딩된 소켓이겠지?
													// 바로 밑 BindAcceptEXSock()에서 바인딩하는 코드가 없음
	m_lpRecvOverlappedEx = new OVERLAPPED_EX(this);
	m_lpSendOverlappedEx = new OVERLAPPED_EX(this);
	m_ringRecvBuffer.Create(initConfig.nRecvBufSize * initConfig.nRecvBufCnt);
	m_ringSendBuffer.Create(initConfig.nSendBufSize * initConfig.nSendBufCnt);

	m_nRecvBufferSize = initConfig.nRecvBufSize;
	m_nSendBufferSize = initConfig.nSendBufSize;
	return BindAcceptExSock();
}

bool Connection::BindAcceptExSock()
{
	// 리슨 소켓이 없다면 acceptex에 bind하지 않는다
	if (m_sockListener == 0)
		return true;
	memset(&m_lpRecvOverlappedEx->s_Overlapped, 0, sizeof(OVERLAPPED));
	m_lpRecvOverlappedEx->s_WsaBuf.buf		= m_szAddressBuf;
	m_lpRecvOverlappedEx->s_lpSocketMsg		= &m_lpRecvOverlappedEx->s_WsaBuf.buf[0];
	m_lpRecvOverlappedEx->s_WsaBuf.len		= m_nRecvBufferSize;
	m_lpRecvOverlappedEx->s_eOperation		= OP_ACCEPT;
	m_lpRecvOverlappedEx->s_lpConnection	= this;
	m_socket = WSASocket(AF_INET, SOCK_STREAM, 
		IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET)
	{
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | Connection::BindAcceptExSock() | WSASocket() Failed: error[%u]"
			, GetLastError());
		return false;
	}

	IncrementAcceptIoRefCount();
	DWORD dwBytes = 0;
	BOOL bRet = AcceptEx(m_sockListener, m_socket, 
		m_lpRecvOverlappedEx->s_WsaBuf.buf, 0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		(LPOVERLAPPED)m_lpRecvOverlappedEx
	);

	if (!bRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		DecrementAcceptIoRefCount();
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | Connection::BindAcceptExSock() | AcceptEx() Failed: error[%u]",
			GetLastError());
		return false;
	}
	return true;
}

// 서버에서 Connection 클래스의 객체와 연결된 클라이언트와의 접속을 종료
bool Connection::CloseConnection(bool bForce)
{
	Monitor::Owner lock(m_csConnection);
	{
		struct linger li = { 0, 0 };	// Default: SO_DONTLINGER
		if (bForce)
			li.l_onoff = 1; // SO_LINGER, timeout = 0
		if (IocpServer() != NULL && m_bIsConnect == TRUE)
			IocpServer()->OnClose(this);
		shutdown(m_socket, SD_BOTH);
		setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&li, sizeof(li));
		closesocket(m_socket);

		m_socket = INVALID_SOCKET;
		if (m_lpRecvOverlappedEx != NULL)
		{
			m_lpRecvOverlappedEx->s_dwRemain = 0;
			m_lpRecvOverlappedEx->s_nTotalBytes = 0;
		}

		if (m_lpSendOverlappedEx != NULL)
		{
			m_lpSendOverlappedEx->s_dwRemain = 0;
			m_lpSendOverlappedEx->s_nTotalBytes = 0;
		}
		//connection을 다시 초기화 시켜준다.
		InitializeConnection();
		BindAcceptExSock();
	}
	return true;
}

// Connection 클래스 객체와 연결되어 있는 클라이언트에 패킷을 보내기 위해
// 링 버퍼에서 공간을 얻어오고 헤더(4바이트)에 버퍼 공간의 크기를 설정
char* Connection::PrepareSendPacket(int sLen)
{
	if (m_bIsConnect == false)
		return NULL;
	char* pBuf = m_ringSendBuffer.ForwardMark(sLen);

	if (pBuf == NULL)
	{
		IocpServer()->CloseConnection(this);
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | Connection::PrepareSendPacket() | Socket[%d] SendRingBuffer overflow",
			m_socket);
		return NULL;
	}
	ZeroMemory(pBuf, sLen);
	CopyMemory(pBuf, &sLen, PACKET_SIZE_LENGTH);
	return pBuf;
}

bool Connection::ReleaseSendPacket(LPOVERLAPPED_EX lpSendOverlappedEx)
{
	if (lpSendOverlappedEx == NULL)
		return false;
	m_ringSendBuffer.ReleaseBuffer(m_lpSendOverlappedEx->s_WsaBuf.len);
	lpSendOverlappedEx = NULL;
	return true;
}

// 클라이언트와 접속됐을 때 Connection 클래스의 객체 포인터(Completion Key)와 소켓을
// IO Completion Port 객체와 연결해 Completion 객체를 통해 데이터를 송수신
bool Connection::BindIOCP(HANDLE& hIOCP)
{
	HANDLE hIOCPHandle;
	Monitor::Owner lock(m_csConnection);
	hIOCPHandle = CreateIoCompletionPort((HANDLE)m_socket,
		hIOCP, (ULONG_PTR)(this), 0);

	if (NULL == hIOCPHandle || hIOCP != hIOCPHandle)
	{
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | Connection::BindIOCP() | CreateIoCompletionPort() Failed : %d",
			GetLastError());
		return false;
	}
	m_hIOCP = hIOCP;
	return true;
}

// 수신 링 버퍼에 최대 수신 크기만큼의 버퍼 공간을 확인한 후 데이터 수신을 위해
// 커널에 Overlapped IO 요청을 한다. 커널에 데이터 수신 준비를 알리고 데이터가
// 수신되면 Completion 객체를 통해 해당 Overlapped IO 작업을 처리한다.
bool Connection::RecvPost(const char* pNext, DWORD dwRemain)
{
	int					nRet = 0;
	DWORD				dwFlag = 0;
	DWORD				dwRecvNumBytes = 0;

	if (m_bIsConnect == false || m_lpRecvOverlappedEx == NULL)
		return false;
	m_lpRecvOverlappedEx->s_eOperation = OP_RECV;
	m_lpRecvOverlappedEx->s_dwRemain = dwRemain;
	int nMoveMark = dwRemain - (m_ringRecvBuffer.GetCurrentMark() - pNext);
	m_lpRecvOverlappedEx->s_WsaBuf.len = m_nRecvBufferSize;
	m_lpRecvOverlappedEx->s_WsaBuf.buf =
		m_ringRecvBuffer.ForwardMark(nMoveMark, m_nRecvBufferSize, dwRemain);
	if (m_lpRecvOverlappedEx->s_WsaBuf.buf == NULL)
	{
		IocpServer()->CloseConnection(this);
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | Connection::RecvPost() | Socket[%d] RecvRingBuffer overflow..",
			m_socket);
		return false;
	}
	m_lpRecvOverlappedEx->s_lpSocketMsg =
		m_lpRecvOverlappedEx->s_WsaBuf.buf - dwRemain;

	memset(&m_lpRecvOverlappedEx->s_Overlapped, 0, sizeof(OVERLAPPED));
	IncrementRecvIoRefCount();

	int ret = WSARecv(
		m_socket,
		&m_lpRecvOverlappedEx->s_WsaBuf,
		1,
		&dwRecvNumBytes,
		&dwFlag,
		&m_lpRecvOverlappedEx->s_Overlapped,
		NULL);

	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		DecrementRecvIoRefCount();
		IocpServer()->CloseConnection(this);
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | Connection::RecvPost() | WSARecv() Failed : %u",
			GetLastError());
		return false;
	}
	return true;
}

// 송신 링 버퍼에 최대 송신 크기만큼 데이터를 가져와 커널에 Overlapped IO를 요청한다.
// m_bIsSend가 있는 이유는 한 연결에 중복으로 데이터 송신을 막기 위함. 
// 예를 들어 1024바이트 데이터를 송신하려고 Overlapped IO를 커널에 요청했지만
// 1000바이트만 송신된 상황에서, WSASend()를 다시 호출해 나머지 24바이트를 보내면 된다.
// 만약 이 작업 사이에 512 바이트를 보내는 작업을 처리해버리면 수신자 입장에서는
// 1000- 512 - 24의 데이터를 받는다.
bool Connection::SendPost(int nSendSize)
{
	DWORD dwBytes;
	//보내는 양이 있다면, 즉 IocpServer class의 DoSend()에서 불린게 아니라 PrepareSendPacket()함수를 부르고 
	//SendPost가 불렸다면 보내는 양이 있고 DoSend에서 불렸다면 0이 온다.
	if (nSendSize > 0)
		m_ringSendBuffer.SetUsedBufferSize(nSendSize);

	// m_bIsSend == TRUE(comperand)이면 m_bIsSend를 FALSE(exchange)로 만들지만
	// 리턴값은 원래의 m_bIsSend 값인 TRUE
	// 멀티스레드 환경에서 실행 중인 함수를 실행하지 않도록 하기 위함
	if (InterlockedCompareExchange((LPLONG)&m_bIsSend, FALSE, TRUE) == TRUE)
	{
		int nReadSize;
		char* pBuf = m_ringSendBuffer.GetBuffer(m_nSendBufferSize, &nReadSize);
		if (pBuf == NULL)
		{
			// 원래의 m_bIsSend 값을 리턴하지만
			// 현재 값은 TRUE(Value변수)로 바뀌어 있다
			InterlockedExchange((LPLONG)&m_bIsSend, TRUE);
			return false;
		}

		m_lpSendOverlappedEx->s_dwRemain = 0;
		m_lpSendOverlappedEx->s_eOperation = OP_SEND;
		m_lpSendOverlappedEx->s_nTotalBytes = nReadSize;
		ZeroMemory(&m_lpSendOverlappedEx->s_Overlapped, sizeof(OVERLAPPED));
		m_lpSendOverlappedEx->s_WsaBuf.len = nReadSize;
		m_lpSendOverlappedEx->s_WsaBuf.buf = pBuf;
		m_lpSendOverlappedEx->s_lpConnection = this;

		IncrementSendIoRefCount();

		int ret = WSASend(
			m_socket,
			&m_lpSendOverlappedEx->s_WsaBuf,
			1,
			&dwBytes,
			0,
			&m_lpSendOverlappedEx->s_Overlapped,
			NULL);
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			DecrementSendIoRefCount();
			IocpServer()->CloseConnection(this);
			LOG(LOG_ERROR_NORMAL,
				"[ERROR] socket[%u] WSASend(): SOCKET_ERROR, %u\n",
				m_socket, WSAGetLastError());
			InterlockedExchange((LPLONG)&m_bIsSend, FALSE);
			return false;
		}
	}
	return true;
}

bool Connection::ConnectTo(const char* szIp, unsigned short usPort)
{
	SOCKADDR_IN	si_addr;
	int			nRet  = 0;
	int			nZero = 0;

	// create listen socket.
	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_socket)
	{
		LOG(LOG_ERROR_LOW, "SYSTEM | Connection::ConnectTo() | WSASocket() , Socket Creation Failed : LastError(%u)", GetLastError());
		return false;
	}

	// bind listen socket with si_addr struct.
	ZeroMemory(&si_addr, sizeof(si_addr));
	si_addr.sin_family		= AF_INET;
	si_addr.sin_port		= htons(usPort);
	si_addr.sin_addr.s_addr = inet_addr(szIp);

	nRet = WSAConnect(m_socket, (sockaddr*)& si_addr, sizeof(sockaddr), NULL, NULL, NULL, NULL);
	if (SOCKET_ERROR == nRet)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | Connection::ConnectTo() | WSAConnect() , WSAConnect Failed : LastError(%u)",
			GetLastError());
		return false;
	}

	HANDLE	hIOCP = IocpServer()->GetWorkerIOCP();
	if (BindIOCP(hIOCP) == false)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | Connection::ConnectTo() | BindIOCP() , BindIOCP Failed : LastError(%u)",
			GetLastError());
		return false;
	}

	m_bIsConnect = TRUE;

	if (RecvPost(m_ringRecvBuffer.GetBeginMark(), 0) == false)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | Connection::ConnectTo() | RecvPost() , BindRecv Failed : LastError(%u)",
			GetLastError());
		return false;
	}
	return true;
}