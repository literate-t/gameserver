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

// ������ ������ ��			  // ���� ������ ����
char* RingBuffer::ForwardMark(int nForwardLength)
{
	char* pPreCurrentMark = NULL;
	Monitor::Owner lock(m_csRingBuffer);

	// �� ���� �����÷ο� Ȯ��
	if (m_nUsedBufferSize + nForwardLength > m_nBufferSize)
		return NULL;
	if ((m_pEndMark - m_pCurrentMark) >= nForwardLength)
	{
		pPreCurrentMark = m_pCurrentMark;
		m_pCurrentMark += nForwardLength;
	}
	else
	{
		// ��ȯ�Ǳ� �� ������ ��ǥ�� ����
		m_pLastMoveMark = m_pCurrentMark;
		// ���� �ּ�ȭ. �ϳ��� ��Ŷ�� �и����� �ʰ�..
		m_pCurrentMark = m_pBeginMark + nForwardLength; // ���� ���ۿ� �ִ� �����ʹ�? �̹� ���۵�? �� �̾ ���� �� �ϰ� ó������ ����?
		pPreCurrentMark = m_pBeginMark;
	}
	//m_nUsedBufferSize += nForwardLength
	return pPreCurrentMark;
}

// ������ ������ ��			  // ���� ���� ������ // ������ ���� ������  
char* RingBuffer::ForwardMark(int nForwardLength, int nNextLength, DWORD dwRemainLength)
{																   // ������� ���� ��Ŷ ����
	Monitor::Owner lock(m_csRingBuffer);

	//������ �����÷� üũ
	if (m_nUsedBufferSize + nForwardLength + nNextLength > m_nBufferSize)
		return NULL;

	if ((m_pEndMark - m_pCurrentMark) > (nNextLength + nForwardLength))
		m_pCurrentMark += nForwardLength;

	/*
		���� ������ ũ�⺸�� ������ ���� �޽����� ũ�Ⱑ �� ũ��
		���� �޽����� ó������ �����Ѵ��� ��ȯ �ȴ�.
	*/
	else
	{
		//��ȯ �Ǳ� �� ������ ��ǥ�� ����
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

	//nBackwardLength��ŭ ���� ���������͸� �ڷ� ������
	m_nUsedBufferSize -= nBackwardLength;
	m_pCurrentMark -= nBackwardLength;
}

void RingBuffer::ReleaseBuffer(int nReleaseSize)
{
	Monitor::Owner lock(m_csRingBuffer);
	m_nUsedBufferSize -= nReleaseSize; // ����� ���� ����� ���̱⸸ �ؼ�?
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

	// ���������� �� �о��ٸ� �� �о���� ������ �����ʹ� �Ǿ����� �ű��
	if (m_pLastMoveMark == m_pGetBufferMark)
	{
		m_pGetBufferMark = m_pBeginMark;
		m_pLastMoveMark = m_pEndMark;
	}

	// ���� ���ۿ� �ִ� size�� �о���� size���� ũ�ٸ�
	if (m_nUsedBufferSize > nReadSize)
	{
		// �������� ������ �Ǵ�
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
		// �������� ������ �Ǵ�
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