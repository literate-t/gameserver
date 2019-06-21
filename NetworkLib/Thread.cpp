#include "Precompile.h"
#include "Thread.h" // Precompile.h에 안 넣고 따로 인클루드?
#include "Log.h"

Thread::Thread(void)
{
	m_hThread		= NULL;
	m_bIsRun		= false;
	m_dwWaitTick	= 0;
	m_dwTickCount	= 0;
	m_hQuitEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);
}

Thread::~Thread(void)
{
	CloseHandle(m_hQuitEvent);
	if (m_hThread)
		CloseHandle(m_hThread);
}

unsigned int WINAPI CallTickThread(LPVOID p)
{
	Thread* pTickThread = (Thread*)p;
	pTickThread->TickThread();
	return 1;
}

bool Thread::CreateThread(DWORD dwWaitTick)
{
	unsigned int uiThreadId = 0;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CallTickThread, this, CREATE_SUSPENDED, &uiThreadId);
	if (m_hThread == NULL)
	{
		LOG(LOG_ERROR_NORMAL, "SYSTEM | Thread::CreateTickThread() | TickThread 생성 실패 : Error(%u) "
			, GetLastError());
		return false;
	}
	m_dwWaitTick = dwWaitTick;
	return true;
}

void Thread::Run()
{
	if (m_bIsRun == false)
	{
		m_bIsRun = true;
		ResumeThread(m_hThread);
	}
}

void Thread::Stop()
{
	if (m_bIsRun == true )
	{
		m_bIsRun = false;
		SuspendThread(m_hThread);
	}
}

void Thread::TickThread()
{
	while (true)
	{
		DWORD dwRet = WaitForSingleObject(m_hQuitEvent, m_dwWaitTick);
		if (WAIT_OBJECT_0 == dwRet)
			break;
		else if (WAIT_TIMEOUT == dwRet)
		{
			m_dwTickCount++;
			OnProcess();
		}
	}
}

void Thread::DestroyThread()
{
	Run();
	SetEvent(m_hQuitEvent);
	WaitForSingleObject(m_hThread, INFINITE);
}