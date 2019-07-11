using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace chat_client {
    class PacketBufferManager {
        int BufferSize = 0;
        int ReadPos = 0;
        int WritePos = 0;

        int HeaderSize = 0;
        int MaxPacketSize = 0;
        byte[] PacketData;
        
        public bool Init(int size, int headerSize, int maxPacketSize) {
            if (size < (maxPacketSize * 2) || size < 1 || headerSize < 1 || maxPacketSize < 1)
                return false;
            BufferSize = size;
            PacketData = new byte[size];
            HeaderSize = headerSize;
            MaxPacketSize = maxPacketSize;

            return true;
        }

        public bool Write(byte[] data, int pos, int size) {
            if (data == null)
                return false;

            var remainingBufferSize = BufferSize - WritePos;
            if (remainingBufferSize < size)
                return false;

            Buffer.BlockCopy(data, pos, PacketData, WritePos, size);
            WritePos += size;
            if (IsNextFree() == false)
                BufferReset();
            return true;
        }

        public ArraySegment<byte> Read() {
            var EnableReadSize = WritePos - ReadPos;
            if (EnableReadSize < HeaderSize) {
                // 이걸 리턴하면 무의미하게 메모리 낭비가 되는 것 같은데
                return new ArraySegment<byte>();
            }
            var PacketDataSize = BitConverter.ToInt16(PacketData, ReadPos);
            if (EnableReadSize < PacketDataSize)
                return new ArraySegment<byte>();
            var CompletePacketData = new ArraySegment<byte>(PacketData, ReadPos, PacketDataSize);
            ReadPos += PacketDataSize;
            return CompletePacketData;
        }

        private bool IsNextFree() {
            var enableWriteSize = BufferSize - WritePos;
            if (enableWriteSize < MaxPacketSize)
                return false;
            return true;
        }

        private void BufferReset() {

        }
    }
}
