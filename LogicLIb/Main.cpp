#include "Main.h"

namespace NLogicLib
{
	Main::Main()
	{
		m_pNetwork		= nullptr;
		m_pPacketProc	= nullptr;
	}	 
	Main::~Main()
	{
		Release();
	}

	void Main::Init()
	{
		m_pNetwork		= std::make_unique<TcpNetwork>();
		m_pPacketProc	= std::make_unique<PacketProcess>();
		m_pNetwork->Init(100);
		m_pNetwork->InitSocket();
		m_pNetwork->BindAndListen(8000);
		m_pNetwork->StartServer();		
		m_pPacketProc->Init(m_pNetwork.get());
		m_IsRun = true;
	}

	void Main::Run()
	{
		while (m_IsRun)
		{
			auto packet = m_pNetwork->GetPacket();
			if (packet.Id == 0)
				continue;
			else
				m_pPacketProc->Process(packet);
		}
	}

	void Main::Stop()
	{
		m_IsRun = false;
	}

	void Main::Release()
	{
		if (m_pNetwork != nullptr)
			m_pNetwork->Release();
	}
}