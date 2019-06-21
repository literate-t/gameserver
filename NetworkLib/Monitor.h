#pragma once
class Monitor
{
private:
	CRITICAL_SECTION	m_csSyncObject;
	Monitor(const Monitor& rhs);
	Monitor& operator=(const Monitor& rhs);

public:
	class Owner
	{
	public:
		explicit Owner(Monitor& crit);
		virtual ~Owner();
	private:
		Monitor& m_csSyncObject;

		// No Copy
		Owner(const Owner& rhs);
		Owner& operator=(const Owner& rhs);
	};
	Monitor();
	virtual ~Monitor();

#if(_WIN32_WINNT >= 0x0400)
	BOOL TryEnter();
#endif
	void Enter();
	void Leave();
};