#pragma once

template < typename T >
class Queue : public Monitor
{
public:
	Queue(int nMaxSize = MAX_QUEUESIZE);
	virtual ~Queue(void);

	bool	PushQueue(T typeQueueItem);		// ����Ÿ ����
	void	PopQueue();						// ����Ÿ ����

	bool	IsEmptyQueue();					// ť�� ������� �˷��ش�
	T		GetFrontQueue();				// �����͸� ������
	int		GetQueueSize();					// ���� ����ִ� �������� ����
	int		GetQueueMaxSize() { return m_nQueueMaxSize; }  // ť�� �ִ� ũ��
	void	SetQueueMaxSize(int nMaxSize) { m_nQueueMaxSize = nMaxSize; }  // ť�� �ִ� ũ�� ����
	void	ClearQueue();					// ť�� ����ִ� ��� ������ ����

private:
	T* m_arrQueue;
	Monitor			m_csQueue;				// ����ȭ�� ����
	int				m_nQueueMaxSize;		// ť �ִ� ũ��

	int				m_nCurSize;
	int				m_nEndMark;
	int				m_nBeginMark;
};

template <typename T>
Queue<T>::Queue(int nMaxSize)
{
	m_arrQueue = new T[nMaxSize];
	m_nQueueMaxSize = nMaxSize;
	ClearQueue();
}

template <typename T>
Queue<T>::~Queue<T>(void)
{
	delete[] m_arrQueue;
}

template <typename T>
bool Queue<T>::PushQueue(T typeQueueItem)
{
	Monitor::Owner lock(m_csQueue);
	//���س��� ����� �Ѿ��ٸ� ���̻� ť�� ���� �ʴ´�.
	if (m_nCurSize >= m_nQueueMaxSize)
		return false;
	m_nCurSize++;
	if (m_nEndMark == m_nQueueMaxSize)
		m_nEndMark = 0;
	m_arrQueue[m_nEndMark++] = typeQueueItem;
	return true;
}

template <typename T>
T Queue<T>::GetFrontQueue()
{
	T typeQueueItem;
	Monitor::Owner lock(m_csQueue);
	{
		if (m_nCurSize <= 0)
			return NULL;
		if (m_nBeginMark == m_nQueueMaxSize)
			m_nBeginMark = 0;
		typeQueueItem = m_arrQueue[m_nBeginMark++];
	}
	return typeQueueItem;
}

template <typename T>
void Queue<T>::PopQueue()
{
	Monitor::Owner lock(m_csQueue);
	m_nCurSize--;
}

template <typename T>
bool Queue<T>::IsEmptyQueue()
{
	bool bFlag = false;
	Monitor::Owner lock(m_csQueue);
	bFlag = (m_nCurSize > 0) ? true : false;
	return bFlag;
}

template <typename T>
int Queue<T>::GetQueueSize()
{
	int nSize;
	Monitor::Owner lock(m_csQueue);
	nSize = m_nCurSize;
	return nSize; // m_nCurSize�� �����ص� �� �� ������
}

template <typename T>
void Queue<T>::ClearQueue()
{
	Monitor::Owner lock(m_csQueue);
	m_nCurSize = 0;
	m_nEndMark = 0;
	m_nBeginMark = 0;
}