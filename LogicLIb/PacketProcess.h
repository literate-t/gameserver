#pragma once
#include "../NetworkLib/Protocol.h"

namespace NServerLib
{
	class TcpNetwork;
}
namespace NLogicLib 
{
	using PACKET = NServerLib::PACKET;
	using PACKET_ID = NServerLib::PACKET_ID;

	class PacketProcess 
	{
		using TcpNet = NServerLib::TcpNetwork;

	public:
		PacketProcess();
		~PacketProcess();

	private:
		typedef bool (PacketProcess::* PacketFunc)(PACKET);
		PacketFunc PacketFuncArray[(int)PACKET_ID::MAX];
		TcpNet* m_pRefNetwork;

	public:
		void Init(TcpNet* pNetwork);
		void Process(PACKET packet);
		bool RoomChatting(PACKET packet);
	};
}