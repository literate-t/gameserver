#pragma once
class RingBuffer : public Monitor
{
private:
	char*	m_pRingBuffer;
	char*	m_pBeginMark;
	char*	m_pEndMark;
	char*	m_pCurrentMark;			// 버퍼의 현재 부분을 가리키는 포인터
	char*	m_pGetBufferMark;		// 현재까지 데이터를 읽은 버퍼의 포인터(?)
	char*	m_pLastMoveMark;

	int		m_nBufferSize;
	int		m_nUsedBufferSize;
	UINT	m_uiAllUserBufferSize;
	Monitor m_csRingBuffer;

private:
	// No Copy
	RingBuffer(const RingBuffer& rhs);
	RingBuffer& operator=(const RingBuffer& rhs);

public:
	RingBuffer(void);
	virtual ~RingBuffer(void);

	// 링버퍼 메모리 할당
	bool			Create(int nBufferSize = MAX_RBUFSIZE);

	// 초기화
	bool			Initialize();

	// 할당된 버퍼 크기를 반환
	inline int		GetBufferSize() { return m_nBufferSize; }

	// 해당하는 버퍼의 포인터를 반환
	inline char*	GetBeginMark() { return m_pBeginMark; }
	inline char*	GetCurrentMark() { return m_pCurrentMark; }
	inline char*	GetEndMark() { return m_pEndMark; }

	// 
	char*			ForwardMark(int nForwardLength);
	char*			ForwardMark(int nForwardLength, int nNextLength, DWORD dwRemainLength);
	void			BackwardMark(int nBackwardLength);

	// 사용된 버퍼 해제
	void			ReleaseBuffer(int nReleaseSize);

	// 사용된 버퍼 크기 반환
	int				GetUsedBufferSize() { return m_nUsedBufferSize; }

	// 사용한 버퍼 크기 설정
	/*
		SendPost()가 멀티 쓰레드로 작동되는데 PrepareSendPacket() 안에서 ForwardMark()가
		버퍼를 늘리면 PrepareSendPacket()를 호출하고 나서 데이터를 채워 넣기도 전에
		다른 쓰레드에서 SendPost()를 호출하면 쓰레기 데이터가 전송될 수 있다.
		이를 방지하기 위해 버퍼에 데이터를 다 넣고 난 뒤에만 사용된 버퍼를 설정할 수 있어야 한다.
		이 함수는 SendPost() 안에서 호출된다.
	*/
	void			SetUsedBufferSize(int UserdBufferSize);

	// 버퍼의 누적 크기 반환
	int				GetAllUsedBufferSize() { return m_uiAllUserBufferSize; }

	// 버퍼의 데이터 반환
	char*			GetBuffer(int nReadSize, int* pReadSize);
};