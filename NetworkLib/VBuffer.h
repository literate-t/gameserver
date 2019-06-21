#pragma once

class VBuffer : public Singleton
{
	DECLARE_SINGLETON(VBuffer);

public:
	VBuffer(int nMaxBufSize = 1024 * 50);
	virtual ~VBuffer(void);

	void			GetChar(char& cCh);
	void			GetShort(short& sNum);
	void			GetInteger(int& nNum);
	void			GetString(char* pszBuffer);
	void			GetStream(char* pszBuffer, short sLen);
	void			SetInteger(int nI);
	void			SetShort(short sShort);
	void			SetChar(char cCh);
	void			SetString(char* pszBuffer);
	void			SetStream(char* pszBuffer, short sLen);
	void			SetBuffer(char* pVBuffer);

	inline int		GetMaxBufferSize() { return m_nMaxBufferSize; }
	inline int		GetCurBufferSize() { return m_nCurBufferSize; }
	inline char*	GetCurMark()	{ return m_pCurMark; }
	inline char*	GetBeginMark() { return m_pszVBuffer; }

	bool			CopyBuffer(char* pDestBuffer);
	void			Init();

private:
	char* m_pszVBuffer;		//���� ����
	char* m_pCurMark;		//���� ���� ��ġ

	int	m_nMaxBufferSize;		//�ִ� ���� ������
	int m_nCurBufferSize;		//���� ���� ���� ������

	VBuffer(const VBuffer& rhs);
	VBuffer& operator=(const VBuffer& rhs);
};
CREATE_FUNCTION(VBuffer, g_GetVBuffer);