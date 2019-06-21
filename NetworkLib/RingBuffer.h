#pragma once
class RingBuffer : public Monitor
{
private:
	char*	m_pRingBuffer;
	char*	m_pBeginMark;
	char*	m_pEndMark;
	char*	m_pCurrentMark;			// ������ ���� �κ��� ����Ű�� ������
	char*	m_pGetBufferMark;		// ������� �����͸� ���� ������ ������(?)
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

	// ������ �޸� �Ҵ�
	bool			Create(int nBufferSize = MAX_RBUFSIZE);

	// �ʱ�ȭ
	bool			Initialize();

	// �Ҵ�� ���� ũ�⸦ ��ȯ
	inline int		GetBufferSize() { return m_nBufferSize; }

	// �ش��ϴ� ������ �����͸� ��ȯ
	inline char*	GetBeginMark() { return m_pBeginMark; }
	inline char*	GetCurrentMark() { return m_pCurrentMark; }
	inline char*	GetEndMark() { return m_pEndMark; }

	// 
	char*			ForwardMark(int nForwardLength);
	char*			ForwardMark(int nForwardLength, int nNextLength, DWORD dwRemainLength);
	void			BackwardMark(int nBackwardLength);

	// ���� ���� ����
	void			ReleaseBuffer(int nReleaseSize);

	// ���� ���� ũ�� ��ȯ
	int				GetUsedBufferSize() { return m_nUsedBufferSize; }

	// ����� ���� ũ�� ����
	/*
		SendPost()�� ��Ƽ ������� �۵��Ǵµ� PrepareSendPacket() �ȿ��� ForwardMark()��
		���۸� �ø��� PrepareSendPacket()�� ȣ���ϰ� ���� �����͸� ä�� �ֱ⵵ ����
		�ٸ� �����忡�� SendPost()�� ȣ���ϸ� ������ �����Ͱ� ���۵� �� �ִ�.
		�̸� �����ϱ� ���� ���ۿ� �����͸� �� �ְ� �� �ڿ��� ���� ���۸� ������ �� �־�� �Ѵ�.
		�� �Լ��� SendPost() �ȿ��� ȣ��ȴ�.
	*/
	void			SetUsedBufferSize(int UserdBufferSize);

	// ������ ���� ũ�� ��ȯ
	int				GetAllUsedBufferSize() { return m_uiAllUserBufferSize; }

	// ������ ������ ��ȯ
	char*			GetBuffer(int nReadSize, int* pReadSize);
};