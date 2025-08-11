<template>
  <a-card class="push-container">
    <template #title>
      <a-space align="center">
        <icon-desktop />
        <h2>推流端</h2>
      </a-space>
    </template>
    
    <a-descriptions :column="1" bordered>
      <a-descriptions-item label="UID">{{ uid }}</a-descriptions-item>
      <a-descriptions-item label="流名称">{{ streamName }}</a-descriptions-item>
      <a-descriptions-item label="音频">{{ audio }}</a-descriptions-item>
      <a-descriptions-item label="视频">{{ video }}</a-descriptions-item>
    </a-descriptions>
    
    <a-divider />
    
    <a-space direction="vertical" fill>
      <a-alert v-if="tips1" :content="tips1" :type="tips1.includes('成功') ? 'success' : 'error'" />
      <a-alert v-if="tips2" :content="tips2" type="info" />
      <a-alert v-if="tips3" :content="tips3" :type="tips3.includes('成功') ? 'success' : 'error'" />
    </a-space>
    
    <a-divider />
    
    <div class="video-container">
      <video ref="localVideo" controls autoplay playsinline class="video-element"></video>
    </div>
    
    <a-divider />
    
    <a-space>
      <a-button type="primary" @click="startPush" :loading="loading">
        <template #icon>
          <icon-play-circle />
        </template>
        开始推流
      </a-button>
      <a-button type="outline" status="danger" @click="stopPush">
        <template #icon>
          <icon-stop />
        </template>
        停止推流
      </a-button>
    </a-space>
    
    <a-divider />

    <a-card title="实时聊天" class="chat-card">
      <div class="chat-messages">
        <div v-for="(msg, index) in chatMessages" :key="index" class="message" :class="msg.type">
          <div class="message-header">
            <span class="sender">{{ msg.sender }}</span>
            <span class="time">{{ msg.time }}</span>
          </div>
          <div class="message-content">{{ msg.content }}</div>
        </div>
      </div>
      
      <a-space class="chat-input">
        <a-input v-model="chatMessage" placeholder="输入消息..." @press-enter="sendChatMessage" />
        <a-button type="primary" @click="sendChatMessage">发送</a-button>
      </a-space>
    </a-card>

    <a-divider />
    
    <a-card title="调试信息" class="debug-card">
      <div class="debug-info">
        <a-list :bordered="false">
          <a-list-item v-for="(entry, index) in debugEntries" :key="index">
            <a-list-item-meta>
              <template #description>
                <span class="time">[{{ entry.time }}]</span>
                <span class="message">{{ entry.message }}</span>
              </template>
            </a-list-item-meta>
          </a-list-item>
        </a-list>
      </div>
    </a-card>
  </a-card>
</template>

<script>
import { Message } from '@arco-design/web-vue';
import $ from 'jquery';
import 'webrtc-adapter';

export default {
props: {
  uid: {
    type: String,
    default: '57901'
  },
  streamName: {
    type: String,
    default: '123.60.30.56'
  },
  audio: {
    type: String,
    default: '1'
  },
  video: {
    type: String,
    default: '1'
  }
},
data() {
  return {
    pc: null,
    offer: null,
    localStream: null,
    lastConnectionState: "",
    tips1: "",
    tips2: "",
    tips3: "",
    debugEntries: [],
    statsInterval: null,
    loading: false,
    chatMessage: "",
      chatMessages: [],
  };
},
mounted() {
  window.addEventListener("message", this.handleWindowMessage);
},
beforeUnmount() {
  this.stopPush();
  window.removeEventListener("message", this.handleWindowMessage);
},
methods: {
  addDebugInfo(message) {
    const timestamp = new Date().toLocaleTimeString();
    this.debugEntries.push({
      time: timestamp,
      message,
    });
    
    // 自动滚动到底部
    this.$nextTick(() => {
      const debugInfo = this.$el.querySelector('.debug-info');
      if (debugInfo) {
        debugInfo.scrollTop = debugInfo.scrollHeight;
      }
    });
  },

  onDataChannelOpen() {
    this.addDebugInfo("DataChannel已打开");
    console.log('DataChannel opened, readyState:', this.dataChannel.readyState);
    console.log('DataChannel protocol:', this.dataChannel.protocol);
    console.log('DataChannel id:', this.dataChannel.id);
    Message.success("DataChannel连接已建立");
  },
  
  onDataChannelMessage(event) {
    const message = event.data;
    this.addDebugInfo(`收到DataChannel消息: ${message}`);
    this.addChatMessage("对方", message);
  },

  // 新增：添加聊天消息
  addChatMessage(sender, content) {
    const timestamp = new Date().toLocaleTimeString();
    this.chatMessages.push({
      sender,
      content,
      time: timestamp,
      type: sender === "我" ? "local" : "remote"
    });
      
    // 滚动到底部
    this.$nextTick(() => {
      const chatMessages = this.$el.querySelector('.chat-messages');
      if (chatMessages) {
        chatMessages.scrollTop = chatMessages.scrollHeight;
      }
    });
  },
    
  // 新增：发送聊天消息
  sendChatMessage() {
    if (!this.chatMessage.trim()) {
      Message.warning("消息不能为空");
      return;
    }
      
    if (!this.dataChannel || this.dataChannel.readyState !== 'open') {
      Message.warning("DataChannel未打开，无法发送消息");
      return;
    }
      
    try {
      // 发送消息
      this.dataChannel.send(this.chatMessage);
        
      // 添加到聊天记录
      this.addChatMessage("我", this.chatMessage);
        
      // 清空输入框
      this.chatMessage = "";
    } catch (error) {
      Message.error(`发送消息失败: ${error.message}`);
      this.addDebugInfo(`发送消息失败: ${error.message}`);
    }
  },

  createDataChannel() {
    if (!this.pc) {
      this.addDebugInfo("无法创建DataChannel: PeerConnection未初始化");
      return;
    }
    
    try {
      // 创建DataChannel
      this.addDebugInfo("尝试创建DataChannel...");
      this.dataChannel = this.pc.createDataChannel("chat", {
        reliable: true, // 可靠传输
        // ordered: false, // 是否保证消息顺序
        // maxRetransmits: 0,    // 无重传
      });
      
      // 设置事件监听
      this.dataChannel.onopen = this.onDataChannelOpen;
      this.dataChannel.onmessage = this.onDataChannelMessage;
      
      this.addDebugInfo("DataChannel已创建");
    } catch (error) {
      this.addDebugInfo(`创建DataChannel失败: ${error.message}`);
    }
  },

  startPush() {
    this.loading = true;
    this.addDebugInfo("发起推流请求");
    
    // 使用jQuery发送请求
    $.post("/signaling/push", {
      uid: this.uid,
      streamName: this.streamName,
      audio: this.audio,
      video: this.video
    }, (data, textStatus) => {
      if (textStatus === "success" && data.errNo === 0) {
        this.tips1 = "<font color='blue'>推流请求成功！</font>";
        this.offer = data.data;
        
        // 调试信息 - 检查offer SDP
        const hasVideo = data.data.sdp.includes("m=video");
        this.addDebugInfo(`推流请求成功，SDP包含视频: ${hasVideo}`);
        
        this.pushStream();
      } else {
        this.tips1 = "<font color='red'>推流请求失败！</font>";
        this.addDebugInfo("推流请求失败");
      }
      this.loading = false;
    }, "json");
  },

  stopPush() {
    this.addDebugInfo("停止推流");
    
    if (this.$refs.localVideo) {
      this.$refs.localVideo.srcObject = null;
    }
    
    if (this.localStream) {
      this.localStream.getTracks().forEach(track => track.stop());
      this.localStream = null;
    }
    
    if (this.pc) {
      this.pc.close();
      this.pc = null;
    }
    
    this.tips1 = "";
    this.tips2 = "";
    this.tips3 = "";
    
    if (this.statsInterval) {
      clearInterval(this.statsInterval);
      this.statsInterval = null;
    }
    
    // 使用jQuery发送停止请求
    $.post("/signaling/stoppush", {
      uid: this.uid,
      streamName: this.streamName
    }, (data, textStatus) => {
      if (textStatus === "success" && data.errNo === 0) {
        this.tips1 = "<font color='blue'>停止推流请求成功！</font>";
      } else {
        this.tips1 = "<font color='red'>停止推流请求失败！</font>";
      }
    }, "json");
  },

  sendAnswer(answerSdp) {
    this.addDebugInfo("发送answer");
    
    // 调试信息 - 检查answer SDP
    const hasVideo = answerSdp.includes("m=video");
    this.addDebugInfo(`发送answer，SDP包含视频: ${hasVideo}`);

    // 使用jQuery发送answer
    $.post("/signaling/sendanswer", {
      uid: this.uid,
      streamName: this.streamName,
      answer: answerSdp,
      type: "push"
    }, (data, textStatus) => {
      if (textStatus === "success" && data.errNo === 0) {
        this.tips3 = "<font color='blue'>answer发送成功！</font>";
        this.addDebugInfo("answer发送成功");
        
        // 定期检查状态
        this.statsInterval = setInterval(() => {
          this.logTrackStatus();
          this.logStats();
        }, 3000);
      } else {
        this.tips3 = "<font color='red'>answer发送失败！</font>";
        this.addDebugInfo("answer发送失败");
      }
    }, "json");
  },

  pushStream() {
    this.addDebugInfo("创建PeerConnection");
    this.pc = new RTCPeerConnection({});
    this.createDataChannel();
    
    // 添加ICE候选项收集监听
    this.pc.onicecandidate = (event) => {
      if (event.candidate) {
        this.addDebugInfo(`新增ICE候选项: ${event.candidate.candidate}`);
      }
    };
    
    this.pc.oniceconnectionstatechange = () => {
      let state = "";
      if (this.lastConnectionState !== "") {
        state = `${this.lastConnectionState}->${this.pc.iceConnectionState}`;
      } else {
        state = this.pc.iceConnectionState;
      }
      
      this.tips2 = `连接状态： ${state}`;
      this.lastConnectionState = this.pc.iceConnectionState;
      
      // 调试信息
      this.addDebugInfo(`ICE连接状态变化: ${state}`);
      
      // 连接成功时记录状态
      if (this.pc.iceConnectionState === "connected") {
        this.addDebugInfo("ICE连接已建立");
        this.logTrackStatus();
      }
    };
    
    this.pc.setRemoteDescription(this.offer)
      .then(this.setRemoteDescriptionSuccess)
      .catch(this.setRemoteDescriptionError);
  },

  handleWindowMessage(event) {
    if (event.origin !== window.location.origin) return;
    
    if (event.data.type) {
      if (event.data.type === "SS_DIALOG_SUCCESS") {
        this.addDebugInfo(`用户同意屏幕共享，streamId: ${event.data.streamId}`);
        this.startScreenStreamFrom(event.data.streamId);
      } else if (event.data.type === "SS_DIALOG_CANCEL") {
        this.addDebugInfo("用户取消屏幕共享");
      }
    }
  },

  startScreenStreamFrom(streamId) {
    this.addDebugInfo(`开始屏幕共享，streamId: ${streamId}`);
    
    const constraints = {
      audio: false,
      video: {
        mandatory: {
          chromeMediaSource: "desktop",
          chromeMediaSourceId: streamId,
          maxWidth: window.screen.width,
          maxHeight: window.screen.height
        }
      }
    };

    navigator.mediaDevices.getUserMedia(constraints)
      .then(this.handleSuccess)
      .catch(this.handleError);
  },

  handleSuccess(stream) {
    this.addDebugInfo("获取屏幕共享流成功");
    
    navigator.mediaDevices.getUserMedia({ audio: true })
      .then(audioStream => {
        this.addDebugInfo("获取音频流成功");
        
        // 记录轨道状态
        this.logTrackStatus();
        
        stream.addTrack(audioStream.getAudioTracks()[0]);
        this.$refs.localVideo.srcObject = stream;
        this.localStream = stream;
        
        this.addDebugInfo("将流添加到PeerConnection");
        // 使用 addStream 方法
        this.pc.addStream(stream);
        
        this.addDebugInfo("开始创建answer");
        this.pc.createAnswer()
          .then(this.createSessionDescriptionSuccess)
          .catch(this.createSessionDescriptionError);
      })
      .catch(this.handleError);
  },

  handleError(error) {
    this.addDebugInfo(`获取用户媒体错误: ${error.message}`);
    Message.error(`获取用户媒体错误: ${error.message}`);
  },

  setRemoteDescriptionSuccess() {
    this.addDebugInfo("远端点描述设置成功");
    
    // 检查SDP包含视频信息
    if (this.offer.sdp) {
      const hasVideo = this.offer.sdp.includes("m=video");
      this.addDebugInfo(`远端点描述包含视频: ${hasVideo}`);
    }
    
    window.postMessage({type: "SS_UI_REQUEST", text: "push"}, "*");
  },

  setRemoteDescriptionError(error) {
    this.addDebugInfo(`远端点描述设置错误: ${error.message}`);
    Message.error(`远端点描述设置错误: ${error.message}`);
  },

  createSessionDescriptionSuccess(answer) {
    // 检查answer SDP
    if (answer.sdp) {
      const hasVideo = answer.sdp.includes("m=video");
      this.addDebugInfo(`answer包含视频: ${hasVideo}`);
    }
    
    this.pc.setLocalDescription(answer)
      .then(this.setLocalDescriptionSuccess)
      .catch(this.setLocalDescriptionError);

    this.sendAnswer(answer.sdp);
  },

  createSessionDescriptionError(error) {
    this.addDebugInfo(`创建answer错误: ${error.message}`);
    Message.error(`创建answer错误: ${error.message}`);
  },

  setLocalDescriptionSuccess() {
    this.addDebugInfo("本地描述设置成功");
  },

  setLocalDescriptionError(error) {
    this.addDebugInfo(`本地描述设置错误: ${error.message}`);
    Message.error(`本地描述设置错误: ${error.message}`);
  },

  logTrackStatus() {
    if (!this.localStream) return;
    
    const videoTracks = this.localStream.getVideoTracks();
    const audioTracks = this.localStream.getAudioTracks();
    
    this.addDebugInfo(`轨道状态: 视频=${videoTracks.length} 音频=${audioTracks.length}`);
    
    if (videoTracks.length > 0) {
      const track = videoTracks[0];
      this.addDebugInfo(`视频轨道状态: readyState=${track.readyState}, enabled=${track.enabled}, muted=${track.muted}`);
    }
  },

  async logStats() {
    if (!this.pc) return;
    
    try {
      const stats = await this.pc.getStats();
      let videoStats = null;
      let audioStats = null;
      
      stats.forEach(report => {
        if (report.type === 'outbound-rtp') {
          if (report.kind === 'video') {
            videoStats = report;
          } else if (report.kind === 'audio') {
            audioStats = report;
          }
        }
      });
      
      if (videoStats) {
        this.addDebugInfo(`视频统计: 包=${videoStats.packetsSent}, 字节=${videoStats.bytesSent}`);
      } else {
        this.addDebugInfo('未找到视频发送统计');
      }
      
      if (audioStats) {
        this.addDebugInfo(`音频统计: 包=${audioStats.packetsSent}, 字节=${audioStats.bytesSent}`);
      }
    } catch (error) {
      this.addDebugInfo(`获取统计失败: ${error.message}`);
    }
  }
}
};
</script>

<style scoped>
.push-container {
max-width: 800px;
margin: 20px auto;
}

.video-container {
margin: 20px 0;
background: #000;
border-radius: 8px;
overflow: hidden;
display: flex;
justify-content: center;
align-items: center;
height: 400px;
}

.video-element {
width: 100%;
height: 100%;
object-fit: contain;
}

.chat-card {
  margin-top: 20px;
}

.chat-messages {
  max-height: 300px;
  overflow-y: auto;
  padding: 10px;
  background-color: #f5f5f5;
  border-radius: 8px;
  margin-bottom: 15px;
}

.message {
  margin-bottom: 10px;
  padding: 8px;
  border-radius: 4px;
}

.message.local {
  background-color: #e6f7ff;
  text-align: right;
}

.message.remote {
  background-color: #f9f9f9;
}

.message-header {
  display: flex;
  justify-content: space-between;
  font-size: 0.8em;
  color: #666;
  margin-bottom: 4px;
}

.message-content {
  word-break: break-word;
}

.chat-input {
  width: 100%;
}

.debug-card {
margin-top: 20px;
}

.debug-info {
max-height: 300px;
overflow-y: auto;
}

.time {
color: var(--color-text-3);
margin-right: 10px;
}

.message {
color: var(--color-text-1);
}
</style>