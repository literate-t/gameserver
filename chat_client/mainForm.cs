using System;
using System.Collections.Generic;
using System.Collections;
using System.Windows.Forms;
using System.Threading;

namespace chat_client {
    public partial class mainForm : Form {

        SimpleTcp Network = new SimpleTcp();
        Queue<byte[]> SendPacketQueue = new Queue<byte[]>();
        Queue<Packet> RecvPacketQueue = new Queue<Packet>();
        bool IsThreadRunning = false;
        Thread ReadThread = null;
        Thread SendThread = null;
        System.Windows.Threading.DispatcherTimer dispatcherUITimer;

        PacketBufferManager PacketManger = new PacketBufferManager();

        private void MainForm_Load(object sender, EventArgs e) {
            IsThreadRunning = true;
            ReadThread = new Thread(ReadProcess);
            SendThread = new Thread(SendProcess);
            ReadThread.Start();
            SendThread.Start();

            dispatcherUITimer = new System.Windows.Threading.DispatcherTimer();
            dispatcherUITimer.Tick += new EventHandler(BackGroundProcess);
            dispatcherUITimer.Interval = new TimeSpan(0, 0, 0, 0, 100);
            dispatcherUITimer.Start();

            PacketManger.Init((8096 * 10), PacketDef.PACKET_HEADER_SIZE, 1024);
        }

        public mainForm() {
            InitializeComponent();
        }

        private void GroupBox1_Enter(object sender, EventArgs e) {

        }        
        
        private void ConnectBtn_Click(object sender, EventArgs e) {
            string address;
            if (localhostcheck.Checked) {
                address = "127.0.0.1";
                addressText.Enabled = false;
            }
            else
                address = addressText.Text;
            int port = Convert.ToInt32(portText.Text);
            if (Network.Connect(address, port)) {
                connectBtn.Enabled = false;
                disconnectBtn.Enabled = true;
            }
        }

        private void DisconnectBtn_Click(object sender, EventArgs e) {
            setDisconnect();
            Network.Close();
        }

        private void IdText_TextChanged(object sender, EventArgs e) {

        }

        private void AddressText_TextChanged(object sender, EventArgs e) {

        }

        public void setDisconnect() {
            if (connectBtn.Enabled == false) {
                connectBtn.Enabled = true;
                disconnectBtn.Enabled = false;
            }
            // 추가 작업
        }

        // 메시지 전송 버튼
        private void ChatSendBtn_Click(object sender, EventArgs e) {
            // 입력된 값 없이 전송을 누르면 아무 작업도 하지 않는다
            if (chatInText.Text == null)
                return;
            // 클릭하면 텍스트를 가져와 패킷화한다
            var RoomChatPacket = new RoomChatReqPacket();
            RoomChatPacket.SetValue(chatInText.Text);
            PostSendPacket(PACKET_ID.ROOM_CHAT_REQ, RoomChatPacket);
        }

        public void PostSendPacket(PACKET_ID packetID, RoomChatReqPacket packet) {
            if (Network.IsConnected() == false) {
                Console.WriteLine("서버 연결이 되어 있지 않습니다.");
                return;
            }
            //Int16 BodyDataSize = (Int16)bodyData.Length;
            //var PacketSize = (Int16)bodyData.Length;//BodyDataSize + PacketDef.PACKET_HEADER_SIZE;
            var PacketSize = packet.GetMsgLen() + PacketDef.PACKET_HEADER_SIZE;
            var BodyData = packet.GetMsg();

            List<byte> dataSource = new List<byte>();
            dataSource.AddRange(BitConverter.GetBytes((Int16)PacketSize));
            dataSource.AddRange(BitConverter.GetBytes((Int16)packetID));
            dataSource.AddRange(BodyData);
            SendPacketQueue.Enqueue(dataSource.ToArray());
        }

        void ReadProcess() {
            const Int16 PacketHeaderSize = PacketDef.PACKET_HEADER_SIZE;
            Int16 len = 0;
            while (IsThreadRunning) {
                if (Network.IsConnected() == false) {
                    Thread.Sleep(1);
                    continue;
                }
                var recvData = Network.Receive(); // Item1: 받은 크기, Item2: 데이터
                if (recvData != null) {
                    PacketManger.Write(recvData.Item2, 0, recvData.Item1);
                    while (true) {
                        var Data = PacketManger.Read();
                        var Packet = new Packet();
                        Packet.DataSize = (Int16)(Data.Count - PacketHeaderSize);
                        Packet.BodyData = new byte[Packet.DataSize];
                        Buffer.BlockCopy(Data.Array, Data.Offset + PacketHeaderSize, Packet.BodyData, 0, Data.Count - PacketHeaderSize);
                        lock (((System.Collections.ICollection)RecvPacketQueue).SyncRoot)
                            RecvPacketQueue.Enqueue(Packet);
                    }
                }
            }
        }

        void SendProcess() {
            while (IsThreadRunning) {
                Thread.Sleep(1);
                if (Network.IsConnected() == false)
                    continue;
                lock(((ICollection)SendPacketQueue).SyncRoot) {
                    if (SendPacketQueue.Count > 0) {
                        var packet = SendPacketQueue.Dequeue();
                        Network.Send(packet);
                    }
                }
            }
        }
        private void BackGroundProcess(object o, EventArgs e) {
            try {
                var Packet = new Packet();
                lock(((System.Collections.ICollection)RecvPacketQueue).SyncRoot) {
                    if (RecvPacketQueue.Count > 0)
                        Packet = RecvPacketQueue.Dequeue();

                }
            } catch (Exception ex){
                MessageBox.Show(string.Format("ReadPacketQueueProcess Error:{0}", ex.Message));
            }
        }

        private void Localhostcheck_CheckedChanged(object sender, EventArgs e) {
            if (localhostcheck.Checked)
                addressText.Enabled = false;
        }
    }
}