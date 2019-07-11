#pragma once
#include "Protocol.h"
class PacketProcess
{
	typedef bool (PacketProcess::*PacketFunc)(PACKET);
	PacketFunc PacketFuncArray[(int)PACKET_ID::MAX];

public:
	void Init();
	void Process(PACKET packet);
	bool RoomChatting(PACKET packet);
};