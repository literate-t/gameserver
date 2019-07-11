#include "PacketProcess.h"

void PacketProcess::Init()
{
	for (int i = 0; i < (int)PACKET_ID::MAX; ++i)
		PacketFuncArray[i] = nullptr;
	PacketFuncArray[(int)PACKET_ID::ROOM_CHAT_REQ] = &PacketProcess::RoomChatting;
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
	return true;
}