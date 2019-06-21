#pragma once

template < typename T >
class Queue : public Monitor
{
public:
	Queue(int nMaxSize = MAX_QUEUESIZE);
	virtual ~Queue(void);

	bool	PushQueue(T typeQueueItem);		// 데이타 삽입
	void	PopQueue();						// 데이타 제거

	bool	IsEmptyQueue();					// 큐가 비었는지 알려준다
	T		GetFrontQueue();				// 데이터를 꺼낸다
	int		GetQueueSize();					// 현재 들어있는 데이터의 갯수
	int		GetQueueMaxSize() { return m_nQueueMaxSize; }  // 큐의 최대 크기
	void	SetQueueMaxSize(int nMaxSize) { m_nQueueMaxSize = nMaxSize; }  // 큐의 최대 크기 설정
	void	ClearQueue();					// 큐에 들어있는 모든 데이터 삭제

private:
	T* m_arrQueue;
	Monitor			m_csQueue;				// 동기화를 위해
	int				m_nQueueMaxSize;		// 큐 최대 크기

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
	//정해놓은 사이즈를 넘었다면 더이상 큐에 넣지 않는다.
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
	return nSize; // m_nCurSize를 리턴해도 될 것 같은데
}

template <typename T>
void Queue<T>::ClearQueue()
{
	Monitor::Owner lock(m_csQueue);
	m_nCurSize = 0;
	m_nEndMark = 0;
	m_nBeginMark = 0;
}