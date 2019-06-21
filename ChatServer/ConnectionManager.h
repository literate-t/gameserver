#pragma once
#include "../NetworkLib/Precompile.h"

class ConnectionManager : public Monitor, Singleton
{
	DECLARE_SINGLETON(ConnectionManager);

public:
	ConnectionManager();
	~ConnectionManager();

	typedef pair<Connection*, DWORD> CONN_PAIR;
	typedef map<Connection*, DWORD>  CONN_MAP;
	typedef CONN_MAP::iterator		 CONN_IT;

	bool CreateConnection(INITCONFIG& initConfig, DWORD dwMaxConnection);
	bool AddConnection(Connection* pConnection);
	bool RemoveConnection(Connection* pConnection);
	inline int __fastcall GetConnectionCnt() { return (int)m_mapConnection.size(); }

	void BroadCastChat(const char* IP, const char* charMsg);

protected:
	CONN_MAP	m_mapConnection;
	Connection* m_pConnection;
	Monitor		m_csConnection;
};
CREATE_FUNCTION(ConnectionManager, g_ConnManager);