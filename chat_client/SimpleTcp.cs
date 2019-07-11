using System;
using System.Net.Sockets;
using System.Net;

namespace chat_client {
    class SimpleTcp {
        public Socket Sock = null;

        // 소켓 연결
        public bool Connect(string ip, int port) {
            Sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            Sock.Connect(ip, port);
            if (Sock == null || Sock.Connected == false) {
                Console.WriteLine("Connect() error");
                return false;
            }
            return true;
        }

        // 스트림에 쓰기
        public void Send(byte[] sendData) {
            if (Sock != null && Sock.Connected)
                Sock.Send(sendData, sendData.Length, SocketFlags.None);
        }

        public Tuple<int, byte[]> Receive() {
            byte[] ReadBuff = new byte[4096];
            var RecvLen = Sock.Receive(ReadBuff, ReadBuff.Length, SocketFlags.None);
            if (RecvLen == 0)
                return null;
            return Tuple.Create(RecvLen, ReadBuff);
        }

        public void Close() {
            if (Sock != null && Sock.Connected)
                Sock.Close();            
        }

        public bool IsConnected() {
            if (Sock == null) return false;
            return true;
        }
    }
}
