#pragma once
#include "../NetworkLib/TcpNetwork.h"
#include "PacketProcess.h"
#include <memory>

using TcpNetwork = NServerLib::TcpNetwork;
using PacketProcess = NLogicLib::PacketProcess;

namespace NLogicLib
{
	class Main
	{
	public:
		Main();
		~Main();
		void Init();
		void Run();
		void Stop();

	private:
		void Release();

	private:
		bool m_IsRun;
		std::unique_ptr<TcpNetwork>		m_pNetwork;
		std::unique_ptr<PacketProcess>	m_pPacketProc;
	};
}