#define MAX_CHATMSG 1024
#define MAX_IP		20
#define PACKET_HEADER_SIZE 2

//enum ePacket {
//
//	PACKET_CHAT = 1,
//};
#pragma pack(push, 1)
typedef struct _PACKET
{
	short			length; // ��Ŷ��� ũ�� ����
	char			msg[MAX_CHATMSG];
} PACKET;
#pragma pack(pop)