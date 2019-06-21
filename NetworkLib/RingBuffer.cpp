#include "Precompile.h"
#include "RingBuffer.h"

RingBuffer::RingBuffer(void)
{
	m_pRingBuffer			= NULL;
	m_pBeginMark			= NULL;
	m_pEndMark				= NULL;
	m_pCurrentMark			= NULL;
	m_pGetBufferMark		= NULL;
	m_pLastMoveMark			= NULL;
	m_nUsedBufferSize		= 0;
	m_uiAllUserBufferSize	= 0;
	m_nBufferSize			= 0;
}

RingBuffer::~RingBuffer(void)
{
	if (NULL != m_pRingBuffer)
		delete[] m_pRingBuffer;
}

bool RingBuffer::Initialize()
{
	Monitor::Owner lock(m_csRingBuffer);
	m_nUsedBufferSize		= 0;
	m_pCurrentMark			= m_pBeginMark;
	m_pGetBufferMark		= m_pBeginMark;
	m_pLastMoveMark			= m_pEndMark;
	m_uiAllUserBufferSize	= 0;

	return true;
}

bool RingBuffer::Create(int nBufferSize)
{
	if (m_pRingBuffer != NULL)
		delete[] m_pRingBuffer;
	m_pBeginMark = new char[nBufferSize];
	if (m_pBeginMark != NULL)
		return false;

	m_pEndMark = m_pBeginMark + nBufferSize - 1;
	m_nBufferSize = nBufferSize;
	Initialize();
	return true;
}

// 데이터 전송할 때			  // 보낼 데이터 길이
char* RingBuffer::ForwardMark(int nForwardLength)
{
	char* pPreCurrentMark = NULL;
	Monitor::Owner lock(m_csRingBuffer);

	// 링 버퍼 오버플로우 확인
	if (m_nUsedBufferSize + nForwardLength > m_nBufferSize)
		return NULL;
	if ((m_pEndMark - m_pCurrentMark) >= nForwardLength)
	{
		pPreCurrentMark = m_pCurrentMark;
		m_pCurrentMark += nForwardLength;
	}
	else
	{
		// 순환되기 전 마지막 좌표를 저장
		m_pLastMoveMark = m_pCurrentMark;
		// 복사 최소화. 하나의 패킷을 분리하지 않고..
		m_pCurrentMark = m_pBeginMark + nForwardLength; // 기존 버퍼에 있던 데이터는? 이미 전송됨? 왜 이어서 진행 안 하고 처음부터 새로?
		pPreCurrentMark = m_pBeginMark;
	}
	//m_nUsedBufferSize += nForwardLength
	return pPreCurrentMark;
}

// 데이터 수신할 때			  // 현재 받은 데이터 // 다음에 받을 데이터  
char* RingBuffer::ForwardMark(int nForwardLength, int nNextLength, DWORD dwRemainLength)
{																   // 현재까지 받은 패킷 길이
	Monitor::Owner lock(m_csRingBuffer);

	//링버퍼 오버플로 체크
	if (m_nUsedBufferSize + nForwardLength + nNextLength > m_nBufferSize)
		return NULL;

	if ((m_pEndMark - m_pCurrentMark) > (nNextLength + nForwardLength))
		m_pCurrentMark += nForwardLength;

	/*
		남은 버퍼의 크기보다 앞으로 받을 메시지의 크기가 더 크면
		현재 메시지를 처음으로 복사한다음 순환 된다.
	*/
	else
	{
		//순환 되기 전 마지막 좌표를 저장
		m_pLastMoveMark = m_pCurrentMark;
		CopyMemory(m_pBeginMark,
			m_pCurrentMark - (dwRemainLength - nForwardLength), dwRemainLength);
		m_pCurrentMark = m_pBeginMark + dwRemainLength;
	}
	return m_pCurrentMark;
}

void RingBuffer::BackwardMark(int nBackwardLength)
{
	Monitor::Owner lock(m_csRingBuffer);

	//nBackwardLength만큼 현재 버퍼포인터를 뒤로 보낸다
	m_nUsedBufferSize -= nBackwardLength;
	m_pCurrentMark -= nBackwardLength;
}

void RingBuffer::ReleaseBuffer(int nReleaseSize)
{
	Monitor::Owner lock(m_csRingBuffer);
	m_nUsedBufferSize -= nReleaseSize; // 사용한 버퍼 사이즈를 줄이기만 해서?
}

void RingBuffer::SetUsedBufferSize(int nUsedBufferSize)
{
	Monitor::Owner lock(m_csRingBuffer);
	m_nUsedBufferSize += nUsedBufferSize;
	m_uiAllUserBufferSize += nUsedBufferSize;
}

char* RingBuffer::GetBuffer(int nReadSize, int* pReadSize)
{
	char* pRet = NULL;
	Monitor::Owner lock(m_csRingBuffer);

	// 마지막까지 다 읽었다면 그 읽어들일 버퍼의 포인터는 맨앞으로 옮긴다
	if (m_pLastMoveMark == m_pGetBufferMark)
	{
		m_pGetBufferMark = m_pBeginMark;
		m_pLastMoveMark = m_pEndMark;
	}

	// 현재 버퍼에 있는 size가 읽어들일 size보다 크다면
	if (m_nUsedBufferSize > nReadSize)
	{
		// 링버퍼의 끝인지 판단
		if ((m_pLastMoveMark - m_pGetBufferMark) >= nReadSize)
		{
			*pReadSize = nReadSize;
			pRet = m_pGetBufferMark;
			m_pGetBufferMark += nReadSize;
		}
		else
		{
			*pReadSize = (int)(m_pLastMoveMark - m_pGetBufferMark);
			pRet = m_pGetBufferMark;
			m_pGetBufferMark += *pReadSize;
		}
	}
	else if (m_nUsedBufferSize > 0)
	{
		// 링버퍼의 끝인지 판단
		if ((m_pLastMoveMark - m_pGetBufferMark) >= m_nUsedBufferSize)
		{
			*pReadSize = m_nUsedBufferSize;
			pRet = m_pGetBufferMark;
			m_pGetBufferMark += m_nUsedBufferSize;
		}
		else
		{
			*pReadSize = (int)(m_pLastMoveMark - m_pGetBufferMark);
			pRet = m_pGetBufferMark;
			m_pGetBufferMark += *pReadSize;
		}
	}
	return pRet;
}