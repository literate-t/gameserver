#pragma once
#define MAX_CHATMSG			1024
#define MAX_IP				20
#define PACKET_HEADER_SIZE	4
#include <string.h>

enum class PACKET_ID : short {
	LOGIN_REQ		= 21,
	LOGIN_RES		= 22,
	ROOM_CHAT_REQ	= 76,
	ROOM_CHAT_RES	= 77,
	MAX				= 256
};
#pragma pack(push, 1)
typedef struct _PACKET
{
	short			DataSize;
	short			Id;
	char			msg[MAX_CHATMSG];
	_PACKET() {
		DataSize = 0;
		Id = 0;
		memset(msg, 0, sizeof(msg));
	}
} PACKET;
#pragma pack(pop)