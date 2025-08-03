<template>
  <div class="pull-container">
    <h2>拉流端</h2>
    
    <div class="highlight">
      拉流端基本信息
      <span>
        uid={{ uid }}
        streamName={{ streamName }}
        audio={{ audio }}
        video={{ video }}
      </span>
    </div>
    
    <div class="status-info">
      <span id="tips1" :innerHTML="tips1"></span> <br>
      <span id="tips2" :innerHTML="tips2"></span> <br>
      <span id="tips3" :innerHTML="tips3"></span>
    </div>
    
    <div class="video-container">
      <video id="remoteVideo" ref="remoteVideo" controls autoplay playsinline></video>
    </div>
    
    <div class="controls">
      <button id="pullBtn" @click="startPull">开始拉流</button>
      <button id="stopPullBtn" @click="stopPull">停止拉流</button>
    </div>
    
    <div id="debugInfo" class="debug-info">
      <div v-for="(entry, index) in debugEntries" :key="index" class="entry">
        <span class="time">[{{ entry.time }}]</span>
        <span class="message">{{ entry.message }}</span>
      </div>
    </div>
  </div>
</template>

<script>
import $ from 'jquery';
import 'webrtc-adapter';

export default {
  props: {
    uid: String,
    streamName: String,
    audio: String,
    video: String
  },
  data() {
    return {
      pc: null,
      offer: null,
      remoteStream: null,
      lastConnectionState: "",
      tips1: "",
      tips2: "",
      tips3: "",
      debugEntries: [],
      statsInterval: null
    };
  },
  mounted() {
    this.checkGlobalLibraries();
  },
  beforeUnmount() {
    this.stopPull();
  },
  methods: {
    checkGlobalLibraries() {
      if (typeof $ === 'undefined') {
        console.error('jQuery is not defined!');
        this.addDebugInfo('jQuery未加载，尝试重新加载');
        
        const jqueryScript = document.createElement('script');
        jqueryScript.src = '/static/js/jquery-2.1.1.min.js';
        document.head.appendChild(jqueryScript);
      }
      
      if (typeof adapter === 'undefined') {
        console.error('adapter is not defined!');
        this.addDebugInfo('WebRTC适配器未加载，尝试重新加载');
        
        const adapterScript = document.createElement('script');
        adapterScript.src = '/static/js/adapter.js';
        document.head.appendChild(adapterScript);
      }
    },
    addDebugInfo(message) {
      const timestamp = new Date().toLocaleTimeString();
      this.debugEntries.push({
        time: timestamp,
        message: message
      });
      
      this.$nextTick(() => {
        const debugInfo = document.getElementById('debugInfo');
        if (debugInfo) {
          debugInfo.scrollTop = debugInfo.scrollHeight;
        }
      });
    },
    updateStatus(elementId, html) {
      this.$nextTick(() => {
        if (elementId === 'tips1') this.tips1 = html;
        if (elementId === 'tips2') this.tips2 = html;
        if (elementId === 'tips3') this.tips3 = html;
      });
    },
    startPull() {
      this.addDebugInfo("发起拉流请求");
      
      $.post("/signaling/pull", {
        uid: this.uid,
        streamName: this.streamName,
        audio: this.audio,
        video: this.video
      }, (data, textStatus) => {
        if (textStatus === "success" && data.errNo === 0) {
          this.updateStatus('tips1', "<font color='blue'>拉流请求成功！</font>");
          this.offer = data.data;
          
          // 调试信息 - 检查offer SDP
          const hasVideo = data.data.sdp.includes("m=video");
          this.addDebugInfo(`拉流请求成功，SDP包含视频: ${hasVideo}`);
          
          this.pullStream();
        } else {
          this.updateStatus('tips1', "<font color='red'>拉流请求失败！</font>");
          this.addDebugInfo("拉流请求失败");
        }
      }, "json");
    },
    stopPull() {
      this.addDebugInfo("停止拉流");
      
      if (this.$refs.remoteVideo) {
        this.$refs.remoteVideo.srcObject = null;
      }
      
      if (this.remoteStream) {
        this.remoteStream.getTracks().forEach(track => track.stop());
        this.remoteStream = null;
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
      
      $.post("/signaling/stoppull", {
        uid: this.uid,
        streamName: this.streamName
      }, (data, textStatus) => {
        if (textStatus === "success" && data.errNo === 0) {
          this.updateStatus('tips1', "<font color='blue'>停止拉流请求成功！</font>");
        } else {
          this.updateStatus('tips1', "<font color='red'>停止拉流请求失败！</font>");
        }
      }, "json");
    },
    sendAnswer(answerSdp) {
      this.addDebugInfo("发送answer");
      
      // 调试信息 - 检查answer SDP
      const hasVideo = answerSdp.includes("m=video");
      this.addDebugInfo(`发送answer，SDP包含视频: ${hasVideo}`);

      $.post("/signaling/sendanswer", {
        uid: this.uid,
        streamName: this.streamName,
        answer: answerSdp,
        type: "pull"
      }, (data, textStatus) => {
        if (textStatus === "success" && data.errNo === 0) {
          this.updateStatus('tips3', "<font color='blue'>answer发送成功！</font>");
          this.addDebugInfo("answer发送成功");
          
          // 定期检查状态
          this.statsInterval = setInterval(() => {
            this.logStats();
          }, 3000);
        } else {
          this.updateStatus('tips3', "<font color='red'>answer发送失败！</font>");
          this.addDebugInfo("answer发送失败");
        }
      }, "json");
    },
    pullStream() {
      this.addDebugInfo("创建PeerConnection");
      this.pc = new RTCPeerConnection({});
      
      // 添加ICE候选项收集监听
      this.pc.onicecandidate = (event) => {
        if (event.candidate) {
          this.addDebugInfo("新增ICE候选项: " + event.candidate.candidate);
        }
      };
      
      this.pc.oniceconnectionstatechange = () => {
        let state = "";
        if (this.lastConnectionState !== "") {
          state = this.lastConnectionState + "->" + this.pc.iceConnectionState;
        } else {
          state = this.pc.iceConnectionState;
        }
        
        this.updateStatus('tips2', "连接状态： " + state);
        this.lastConnectionState = this.pc.iceConnectionState;
        
        // 调试信息
        this.addDebugInfo(`ICE连接状态变化: ${state}`);
        
        // 连接成功时记录状态
        if (this.pc.iceConnectionState === "connected") {
          this.addDebugInfo("ICE连接已建立");
        }
      };
      
      this.pc.ontrack = (event) => {
        if (event.streams && event.streams[0]) {
          this.remoteStream = event.streams[0];
          this.$refs.remoteVideo.srcObject = this.remoteStream;
          this.addDebugInfo("接收到远程流");
        }
      };
      
      this.pc.setRemoteDescription(this.offer)
        .then(this.setRemoteDescriptionSuccess)
        .catch(this.setRemoteDescriptionError);
    },
    setRemoteDescriptionSuccess() {
      this.addDebugInfo("远端点描述设置成功");
      
      // 检查SDP包含视频信息
      if (this.offer.sdp) {
        const hasVideo = this.offer.sdp.includes("m=video");
        this.addDebugInfo(`远端点描述包含视频: ${hasVideo}`);
      }
      
      this.addDebugInfo("开始创建answer");
      this.pc.createAnswer()
        .then(this.createSessionDescriptionSuccess)
        .catch(this.createSessionDescriptionError);
    },
    setRemoteDescriptionError(error) {
      this.addDebugInfo(`远端点描述设置错误: ${error.message}`);
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
    },
    setLocalDescriptionSuccess() {
      this.addDebugInfo("本地描述设置成功");
    },
    setLocalDescriptionError(error) {
      this.addDebugInfo(`本地描述设置错误: ${error.message}`);
    },
    async logStats() {
      if (!this.pc) return;
      
      try {
        const stats = await this.pc.getStats();
        let videoStats = null;
        let audioStats = null;
        
        stats.forEach(report => {
          if (report.type === 'inbound-rtp') {
            if (report.kind === 'video') {
              videoStats = report;
            } else if (report.kind === 'audio') {
              audioStats = report;
            }
          }
        });
        
        if (videoStats) {
          this.addDebugInfo(`视频统计: 包=${videoStats.packetsReceived}, 字节=${videoStats.bytesReceived}`);
        } else {
          this.addDebugInfo('未找到视频接收统计');
        }
        
        if (audioStats) {
          this.addDebugInfo(`音频统计: 包=${audioStats.packetsReceived}, 字节=${audioStats.bytesReceived}`);
        }
      } catch (error) {
        this.addDebugInfo(`获取统计失败: ${error.message}`);
      }
    }
  }
};
</script>

<style scoped>
.pull-container {
  font-family: Arial, sans-serif;
  padding: 20px;
  max-width: 800px;
  margin: 0 auto;
  background-color: #f9f9f9;
  border-radius: 8px;
  box-shadow: 0 2px 10px rgba(0,0,0,0.1);
}

.highlight {
  background-color: #eef7ff;
  margin: 0 0 15px 0;
  padding: 10px 15px;
  border-radius: 4px;
  border-left: 4px solid #4a90e2;
}

.status-info {
  margin: 15px 0;
  padding: 10px;
  background-color: #f5f5f5;
  border-radius: 4px;
}

.video-container {
  margin: 15px 0;
  background: #000;
  border-radius: 4px;
  overflow: hidden;
}

video {
  width: 100%;
  height: auto;
  max-height: 400px;
  display: block;
}

.controls {
  margin: 15px 0;
  display: flex;
  gap: 10px;
}

button {
  background-color: #4a90e2;
  border: none;
  border-radius: 4px;
  color: white;
  padding: 10px 15px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.3s;
  flex: 1;
}

button:hover {
  background-color: #357ae8;
}

button#stopPullBtn {
  background-color: #d84a38;
}

button#stopPullBtn:hover {
  background-color: #cf402f;
}

.debug-info {
  height: 200px;
  width: 100%;
  overflow-y: auto;
  border: 1px solid #ddd;
  padding: 10px;
  background: #333;
  color: #fff;
  font-family: monospace;
  margin-top: 15px;
  border-radius: 4px;
}

.debug-info .entry {
  margin-bottom: 5px;
  padding-bottom: 5px;
  border-bottom: 1px solid #555;
}

.debug-info .time {
  color: #0f0;
  margin-right: 10px;
}

.debug-info .message {
  color: #fff;
}
</style>