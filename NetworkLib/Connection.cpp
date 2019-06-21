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
	m_sockListener	= initConfig.sockListener;		// SOCKADDR_IN ������ ���� IP, PORT�� ���ε��� �����̰���?
													// �ٷ� �� BindAcceptEXSock()���� ���ε��ϴ� �ڵ尡 ����
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
	// ���� ������ ���ٸ� acceptex�� bind���� �ʴ´�
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

// �������� Connection Ŭ������ ��ü�� ����� Ŭ���̾�Ʈ���� ������ ����
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
		//connection�� �ٽ� �ʱ�ȭ �����ش�.
		InitializeConnection();
		BindAcceptExSock();
	}
	return true;
}

// Connection Ŭ���� ��ü�� ����Ǿ� �ִ� Ŭ���̾�Ʈ�� ��Ŷ�� ������ ����
// �� ���ۿ��� ������ ������ ���(4����Ʈ)�� ���� ������ ũ�⸦ ����
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

// Ŭ���̾�Ʈ�� ���ӵ��� �� Connection Ŭ������ ��ü ������(Completion Key)�� ������
// IO Completion Port ��ü�� ������ Completion ��ü�� ���� �����͸� �ۼ���
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

// ���� �� ���ۿ� �ִ� ���� ũ�⸸ŭ�� ���� ������ Ȯ���� �� ������ ������ ����
// Ŀ�ο� Overlapped IO ��û�� �Ѵ�. Ŀ�ο� ������ ���� �غ� �˸��� �����Ͱ�
// ���ŵǸ� Completion ��ü�� ���� �ش� Overlapped IO �۾��� ó���Ѵ�.
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

// �۽� �� ���ۿ� �ִ� �۽� ũ�⸸ŭ �����͸� ������ Ŀ�ο� Overlapped IO�� ��û�Ѵ�.
// m_bIsSend�� �ִ� ������ �� ���ῡ �ߺ����� ������ �۽��� ���� ����. 
// ���� ��� 1024����Ʈ �����͸� �۽��Ϸ��� Overlapped IO�� Ŀ�ο� ��û������
// 1000����Ʈ�� �۽ŵ� ��Ȳ����, WSASend()�� �ٽ� ȣ���� ������ 24����Ʈ�� ������ �ȴ�.
// ���� �� �۾� ���̿� 512 ����Ʈ�� ������ �۾��� ó���ع����� ������ ���忡����
// 1000- 512 - 24�� �����͸� �޴´�.
bool Connection::SendPost(int nSendSize)
{
	DWORD dwBytes;
	//������ ���� �ִٸ�, �� IocpServer class�� DoSend()���� �Ҹ��� �ƴ϶� PrepareSendPacket()�Լ��� �θ��� 
	//SendPost�� �ҷȴٸ� ������ ���� �ְ� DoSend���� �ҷȴٸ� 0�� �´�.
	if (nSendSize > 0)
		m_ringSendBuffer.SetUsedBufferSize(nSendSize);

	// m_bIsSend == TRUE(comperand)�̸� m_bIsSend�� FALSE(exchange)�� ��������
	// ���ϰ��� ������ m_bIsSend ���� TRUE
	// ��Ƽ������ ȯ�濡�� ���� ���� �Լ��� �������� �ʵ��� �ϱ� ����
	if (InterlockedCompareExchange((LPLONG)&m_bIsSend, FALSE, TRUE) == TRUE)
	{
		int nReadSize;
		char* pBuf = m_ringSendBuffer.GetBuffer(m_nSendBufferSize, &nReadSize);
		if (pBuf == NULL)
		{
			// ������ m_bIsSend ���� ����������
			// ���� ���� TRUE(Value����)�� �ٲ�� �ִ�
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