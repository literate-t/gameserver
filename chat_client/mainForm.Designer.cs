namespace chat_client {
    partial class mainForm {
        /// <summary>
        /// 필수 디자이너 변수입니다.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 사용 중인 모든 리소스를 정리합니다.
        /// </summary>
        /// <param name="disposing">관리되는 리소스를 삭제해야 하면 true이고, 그렇지 않으면 false입니다.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form 디자이너에서 생성한 코드

        /// <summary>
        /// 디자이너 지원에 필요한 메서드입니다. 
        /// 이 메서드의 내용을 코드 편집기로 수정하지 마세요.
        /// </summary>
        private void InitializeComponent() {
            this.addressText = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.portText = new System.Windows.Forms.TextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.disconnectBtn = new System.Windows.Forms.Button();
            this.connectBtn = new System.Windows.Forms.Button();
            this.idText = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.localhostcheck = new System.Windows.Forms.CheckBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.chatSendBtn = new System.Windows.Forms.Button();
            this.chatInText = new System.Windows.Forms.TextBox();
            this.chatOutText = new System.Windows.Forms.TextBox();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // addressText
            // 
            this.addressText.Location = new System.Drawing.Point(256, 24);
            this.addressText.Name = "addressText";
            this.addressText.Size = new System.Drawing.Size(116, 21);
            this.addressText.TabIndex = 0;
            this.addressText.Text = "0.0.0.0";
            this.addressText.TextChanged += new System.EventHandler(this.AddressText_TextChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(196, 29);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(57, 12);
            this.label1.TabIndex = 1;
            this.label1.Text = "서버 주소";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(14, 56);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(57, 12);
            this.label2.TabIndex = 3;
            this.label2.Text = "포트 번호";
            // 
            // portText
            // 
            this.portText.Location = new System.Drawing.Point(74, 51);
            this.portText.Name = "portText";
            this.portText.Size = new System.Drawing.Size(116, 21);
            this.portText.TabIndex = 2;
            this.portText.Text = "8000";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.disconnectBtn);
            this.groupBox1.Controls.Add(this.connectBtn);
            this.groupBox1.Controls.Add(this.idText);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.addressText);
            this.groupBox1.Controls.Add(this.localhostcheck);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.portText);
            this.groupBox1.Location = new System.Drawing.Point(12, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(381, 115);
            this.groupBox1.TabIndex = 4;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "더미 클라이언트 설정";
            this.groupBox1.Enter += new System.EventHandler(this.GroupBox1_Enter);
            // 
            // disconnectBtn
            // 
            this.disconnectBtn.Location = new System.Drawing.Point(193, 82);
            this.disconnectBtn.Name = "disconnectBtn";
            this.disconnectBtn.Size = new System.Drawing.Size(75, 23);
            this.disconnectBtn.TabIndex = 9;
            this.disconnectBtn.Text = "접속끊기";
            this.disconnectBtn.UseVisualStyleBackColor = true;
            this.disconnectBtn.Click += new System.EventHandler(this.DisconnectBtn_Click);
            // 
            // connectBtn
            // 
            this.connectBtn.Location = new System.Drawing.Point(112, 82);
            this.connectBtn.Name = "connectBtn";
            this.connectBtn.Size = new System.Drawing.Size(75, 23);
            this.connectBtn.TabIndex = 8;
            this.connectBtn.Text = "접속하기";
            this.connectBtn.UseVisualStyleBackColor = true;
            this.connectBtn.Click += new System.EventHandler(this.ConnectBtn_Click);
            // 
            // idText
            // 
            this.idText.Location = new System.Drawing.Point(74, 24);
            this.idText.Name = "idText";
            this.idText.Size = new System.Drawing.Size(116, 21);
            this.idText.TabIndex = 6;
            this.idText.TextChanged += new System.EventHandler(this.IdText_TextChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(14, 29);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(41, 12);
            this.label3.TabIndex = 7;
            this.label3.Text = "아이디";
            // 
            // localhostcheck
            // 
            this.localhostcheck.AutoSize = true;
            this.localhostcheck.Location = new System.Drawing.Point(256, 52);
            this.localhostcheck.Name = "localhostcheck";
            this.localhostcheck.Size = new System.Drawing.Size(75, 16);
            this.localhostcheck.TabIndex = 5;
            this.localhostcheck.Text = "localhost";
            this.localhostcheck.UseVisualStyleBackColor = true;
            this.localhostcheck.CheckedChanged += new System.EventHandler(this.Localhostcheck_CheckedChanged);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.chatSendBtn);
            this.groupBox2.Controls.Add(this.chatInText);
            this.groupBox2.Controls.Add(this.chatOutText);
            this.groupBox2.Location = new System.Drawing.Point(12, 136);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(381, 240);
            this.groupBox2.TabIndex = 9;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "채팅창";
            // 
            // chatSendBtn
            // 
            this.chatSendBtn.Location = new System.Drawing.Point(315, 208);
            this.chatSendBtn.Name = "chatSendBtn";
            this.chatSendBtn.Size = new System.Drawing.Size(57, 23);
            this.chatSendBtn.TabIndex = 2;
            this.chatSendBtn.Text = "send";
            this.chatSendBtn.UseVisualStyleBackColor = true;
            this.chatSendBtn.Click += new System.EventHandler(this.ChatSendBtn_Click);
            // 
            // chatInText
            // 
            this.chatInText.Location = new System.Drawing.Point(6, 209);
            this.chatInText.Name = "chatInText";
            this.chatInText.Size = new System.Drawing.Size(307, 21);
            this.chatInText.TabIndex = 1;
            // 
            // chatOutText
            // 
            this.chatOutText.Location = new System.Drawing.Point(7, 17);
            this.chatOutText.Multiline = true;
            this.chatOutText.Name = "chatOutText";
            this.chatOutText.Size = new System.Drawing.Size(365, 186);
            this.chatOutText.TabIndex = 0;
            // 
            // mainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(406, 384);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Name = "mainForm";
            this.Text = "테스트 클라이언트";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox addressText;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox portText;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.TextBox idText;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.CheckBox localhostcheck;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button chatSendBtn;
        private System.Windows.Forms.TextBox chatInText;
        private System.Windows.Forms.TextBox chatOutText;
        private System.Windows.Forms.Button disconnectBtn;
        private System.Windows.Forms.Button connectBtn;
    }
}

