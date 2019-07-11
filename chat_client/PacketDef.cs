using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace chat_client {

    class PacketDef {
        public const Int16 PACKET_HEADER_SIZE = 2;
    }

    public enum PACKET_ID : ushort {
        LOGIN_REQ = 21,
        LOGIN_RES = 22,
        ROOM_CHAT_REQ = 76,
        ROOM_CHAT_RES = 77
    }
}