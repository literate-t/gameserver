#pragma once
#include "Queue.h"
#include "RingBuffer.h"

class Connection : public Monitor
{
private:
	SOCKET		m_socket;
	//�ѹ��� ������ �� �ִ� �������� �ִ� ũ��
	int			m_nRecvBufferSize;
	//�ѹ��� �۽��� �� �ִ� �������� �ִ� ũ��
	int			m_nSendBufferSize;

	BYTE		m_szIp[MAX_IP_LENGTH];
	int			m_nIndex;

	Monitor		m_csConnection;

	SOCKET		m_sockListener;
	HANDLE		m_hIOCP;
	//Overlapped IO �۾��� ��û�� ����
	DWORD		m_dwSendIoRefCount;
	DWORD		m_dwRecvIoRefCount;
	DWORD		m_dwAcceptIoRefCount;

private:
	Connection(const Connection& rhs);
	Connection& operator=(const Connection& rhs);

public:
	//Overlapped IO ��û�� ���� ����
	LPOVERLAPPED_EX		m_lpRecvOverlappedEx;
	LPOVERLAPPED_EX		m_lpSendOverlappedEx;
	//Ŭ���̾�Ʈ�� ������ �ۼ����� ���� �� ����
	RingBuffer			m_ringRecvBuffer;
	RingBuffer			m_ringSendBuffer;
	//Ŭ���̾�Ʈ �ּҸ� �ޱ����� ����
	char				m_szAddressBuf[1024];
	//Ŭ���̾�Ʈ�� ���� ���ᰡ �Ǿ����� ����
	BOOL				m_bIsClosed;
	//Ŭ���̾�Ʈ�� ������ �Ǿ��ִ��� ����
	BOOL				m_bIsConnect;
	//���� Overlapped I/O ���� �۾��� �ϰ� �ִ��� ����
	BOOL				m_bIsSend;

public:
	Connection(void);
	virtual ~Connection(void);

	void 	InitializeConnection();
	bool 	CreateConnection(INITCONFIG& initConfig);
	bool 	CloseConnection(bool bForce = FALSE);
	bool	ConnectTo(const char* szIp, unsigned short usPort);
	bool 	BindIOCP(HANDLE& hIOCP);
	bool 	RecvPost(const char* pNext, DWORD dwRemain);
	bool 	SendPost(int nSendSize);
	void 	SetSocket(SOCKET socket) { m_socket = socket; }
	SOCKET 	GetSocket() { return m_socket; }
	bool 	BindAcceptExSock();
	char*	PrepareSendPacket(int sLen);
	bool 	ReleaseRecvPacket();
	bool 	ReleaseSendPacket(LPOVERLAPPED_EX lpSendOverlappedEx = NULL);

	inline void  SetConnectionIp(char* Ip) { memcpy(m_szIp, Ip, MAX_IP_LENGTH); }
	inline BYTE* GetConnectionIp()		{ return m_szIp; }

	inline int  GetIndex()				{ return m_nIndex; }
	inline int  GetRecvBufSize()		{ return m_nRecvBufferSize; }
	inline int  GetSendBufSize()		{ return m_nSendBufferSize; }

	inline int  GetRecvIoRefCount()		{ return m_dwRecvIoRefCount; }
	inline int  GetSendIoRefCount()		{ return m_dwSendIoRefCount; }
	inline int  GetAcceptIoRefCount()	{ return m_dwAcceptIoRefCount; }

	inline int  IncrementRecvIoRefCount()
	{
		return InterlockedIncrement((LPLONG)&m_dwRecvIoRefCount);
	}
	inline int  IncrementSendIoRefCount()
	{
		return InterlockedIncrement((LPLONG)&m_dwSendIoRefCount);
	}
	inline int  IncrementAcceptIoRefCount()
	{
		return InterlockedIncrement((LPLONG)&m_dwAcceptIoRefCount);
	}
	inline int  DecrementRecvIoRefCount()
	{
		return (m_dwRecvIoRefCount ? InterlockedDecrement((LPLONG)&m_dwRecvIoRefCount) : 0);
	}
	inline int  DecrementSendIoRefCount()
	{
		return (m_dwSendIoRefCount ? InterlockedDecrement((LPLONG)&m_dwSendIoRefCount) : 0);
	}
	inline int  DecrementAcceptIoRefCount()
	{
		return (m_dwAcceptIoRefCount ? InterlockedDecrement((LPLONG)&m_dwAcceptIoRefCount) : 0);
	}
};