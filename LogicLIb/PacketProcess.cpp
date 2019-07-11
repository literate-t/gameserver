#include "PacketProcess.h"
#include "../NetworkLib/TcpNetwork.h"

namespace NLogicLib 
{
	PacketProcess::PacketProcess() {}
	PacketProcess::~PacketProcess() {}
	void PacketProcess::Init(TcpNet* pNetwork)
	{
		for (int i = 0; i < (int)PACKET_ID::MAX; ++i)
			PacketFuncArray[i] = nullptr;
		PacketFuncArray[(int)PACKET_ID::ROOM_CHAT_REQ] = &PacketProcess::RoomChatting;
		m_pRefNetwork = pNetwork;
	}

	void PacketProcess::Process(PACKET packet)
	{
		auto packetId = packet.Id;
		if (PacketFuncArray[packetId] == nullptr)
			return;
		(this->*PacketFuncArray[packetId])(packet);
	}

	bool PacketProcess::RoomChatting(PACKET packet)
	{
		//m_pRefNetwork->SendData();
		return true;
	}
}