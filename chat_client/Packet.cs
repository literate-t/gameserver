using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace chat_client {

    struct Packet {
        public Int16 DataSize;
        public Int16 PacketID;
        public byte[] BodyData;
    }

    public class RoomChatReqPacket {
        Int16 MsgLen;
        byte[] Msg;
        public Int16 GetMsgLen() {
            return MsgLen;
        }
        public byte[] GetMsg() {
            return Msg;
        }

        public void SetValue(string m) {
            Msg = Encoding.UTF8.GetBytes(m);
            MsgLen = (Int16)Msg.Length;
        }

        public byte[] ToBytes() {
            List<byte> dataSource = new List<byte>();
            dataSource.AddRange(BitConverter.GetBytes(MsgLen));
            dataSource.AddRange(Msg);
            return dataSource.ToArray();
        }
    }
}