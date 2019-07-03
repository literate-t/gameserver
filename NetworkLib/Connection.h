#pragma once
#include "Queue.h"
#include "RingBuffer.h"

class Connection : public Monitor
{
private:
	SOCKET		m_socket;
	// �� ���� ������ �� �ִ� ������ �ִ� ũ��
	int			m_nRecvBuffSize;
	// �� ���� �۽��� �� �ִ� ������ �ִ� ũ��
	int			m_nSendBuffSize;
	BYTE		m_Ip[MAX_IP_LENGTH];
	int			m_nIndex;
	Monitor		m_csConnection;
	SOCKET		m_sockListener;
	HANDLE		m_hIOCP;
	// Overlapped IO �۾��� ��û�� Ƚ��
	DWORD		m_dwSendIoRefCount;
	DWORD		m_dwRecvIoRefCount;
	DWORD		m_dwAcceptIoRefCount;

	Connection(const Connection& rhs);
	Connection& operator=(const Connection& rhs);

public:
	// Overlapped IO ��û�� ���� ����
	LPOVERLAPPED_EX m_lpRecvOverlappedEx;
	LPOVERLAPPED_EX m_lpSendOverlappedEx;
	// Ŭ���̾�Ʈ�� ������ �ۼ����� ���� �� ����
	RingBuffer		m_ringRecvBuffer;
	RingBuffer		m_ringSendBuffer;
	// Ŭ���̾�Ʈ �ּҸ� �ޱ� ���� ����
	char			m_addressBuff[1024];
	// Ŭ���̾�Ʈ�� ������ ����ƴ���
	bool			m_bIsClosed;
	// Ŭ���̾�Ʈ�� ������ �Ǿ� �ִ���
	bool			m_bIsConnected;
	// ���� Overlapped IO ���� �۾��� �ϰ� �ִ���
	bool			m_bIsSent;

public:
	Connection();
	virtual ~Connection();

	void			InitializeConnection();
	bool			CreateConnection(INITCONFIG& initConfig);
	bool			CloseConnection(bool bForce = false);
	bool			BindIOCP(HANDLE& hIOCP);
	bool			RecvPost(const char* pNext, DWORD dwRemain);
	bool			SendPost(int sendPost);	
	void			SetSocket(SOCKET socket) { m_socket = socket; }
	SOCKET			GetSocket() { return m_socket; }
	bool			BindAcceptExSock();
	char*			PrepareSendPacket(const int len);
	bool			ReleaseRecvPacket();
	bool			ReleaseSendPacket(LPOVERLAPPED_EX lpSendOverlappedEx = NULL);

	inline void		SetConnectionIp(const char* Ip) { memcpy(m_Ip, Ip, MAX_IP_LENGTH); }
	inline BYTE*	GetConnectionIp()				{ return m_Ip; }
	inline int		GetIndex()						{ return m_nIndex; }

	inline int		GetRecvBuffSize()				{ return m_nRecvBuffSize; }
	inline int		GetSendBuffsize()				{ return m_nSendBuffSize; }

	inline int		GetRecvIoRefCount()				{ return m_dwRecvIoRefCount; }
	inline int		GetSendIoRefCount()				{ return m_dwSendIoRefCount; }
	inline int		GetAcceptIoRefCount()			{ return m_dwAcceptIoRefCount; }

	inline int  IncrementRecvIoRefCount()
	{
		return InterlockedIncrement((LPLONG)& m_dwRecvIoRefCount);
	}
	inline int  IncrementSendIoRefCount()
	{
		return InterlockedIncrement((LPLONG)& m_dwSendIoRefCount);
	}
	inline int  IncrementAcceptIoRefCount()
	{
		return InterlockedIncrement((LPLONG)& m_dwAcceptIoRefCount);
	}
	inline int  DecrementRecvIoRefCount()
	{
		return (m_dwRecvIoRefCount ? InterlockedDecrement((LPLONG)& m_dwRecvIoRefCount) : 0);
	}
	inline int  DecrementSendIoRefCount()
	{
		return (m_dwSendIoRefCount ? InterlockedDecrement((LPLONG)& m_dwSendIoRefCount) : 0);
	}
	inline int  DecrementAcceptIoRefCount()
	{
		return (m_dwAcceptIoRefCount ? InterlockedDecrement((LPLONG)& m_dwAcceptIoRefCount) : 0);
	}	
};