<template>
  <div class="push-container">
    <h2>推流端</h2>
    
    <div class="highlight">
      推流端基本信息
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
      <video id="localVideo" ref="localVideo" controls autoplay playsinline></video>
    </div>
    
    <div class="controls">
      <button id="pushBtn" @click="startPush">开始推流</button>
      <button id="stopPushBtn" @click="stopPush">停止推流</button>
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
      localStream: null,
      lastConnectionState: "",
      tips1: "",
      tips2: "",
      tips3: "",
      debugEntries: [],
      statsInterval: null
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
        message: message
      });
      
      // 自动滚动到底部
      this.$nextTick(() => {
        const debugInfo = document.getElementById('debugInfo');
        if (debugInfo) {
          debugInfo.scrollTop = debugInfo.scrollHeight;
        }
      });
    },
    startPush() {
      this.addDebugInfo("发起推流请求");
      
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
        
        this.tips2 = "连接状态： " + state;
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

button#stopPushBtn {
  background-color: #d84a38;
}

button#stopPushBtn:hover {
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