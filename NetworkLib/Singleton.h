#pragma once
#define DECLARE_SINGLETON( className )\
public:\
	static className* Instance();\
	virtual void releaseInstance();\
private:\
	static className* m_pInstance;

#define CREATE_FUNCTION( className, funcName ) \
	static className* ##funcName()\
	{\
		return className::Instance();\
	};

#define IMPLEMENT_SINGLETON( className )\
	className* className::m_pInstance = NULL;\
	className* className::Instance()\
	{\
		if ( m_pInstance == NULL)\
		{\
			m_pInstance = new className();\
		}\
		return m_pInstance;\
	}\
	void className::releaseInstance()\
	{\
		if (m_pInstance != NULL)\
		{\
			delete m_pInstance;\
			m_pInstance = NULL;\
		}\
	}

class Singleton
{
public:
	typedef std::list<Singleton*> SINGLETON_LIST;
	Singleton();
	virtual ~Singleton();
	virtual void releaseInstance() = 0;
	static void releaseAll();

private:
	static SINGLETON_LIST m_listSingleton;
};