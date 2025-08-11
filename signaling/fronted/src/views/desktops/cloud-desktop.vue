<template>
  <div class="remote-desktop">
    <a-page-header 
      :title="`远程桌面 - ${desktopName}`" 
      :subtitle="`IP: ${desktopIP} | 操作系统: ${desktopOS}`"
    >
      <template #extra>
        <a-button type="primary" @click="disconnect">
          <template #icon>
            <icon-disconnect />
          </template>
          断开连接
        </a-button>
      </template>
    </a-page-header>
    
    <div class="desktop-container">
      <!-- 全屏显示远程视频 -->
      <video 
        id="remoteVideo" 
        ref="remoteVideo" 
        autoplay 
        playsinline
        class="video-element"
        @mousedown="handleMouseDown"
        @mouseup="handleMouseUp"
        @mousemove="handleMouseMove"
        @contextmenu.prevent="handleContextMenu"
        @keydown="handleKeyDown"
        @keyup="handleKeyUp"
        tabindex="0"
      ></video>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, onMounted, onBeforeUnmount } from 'vue';
import { useRoute } from 'vue-router';
import { Message } from '@arco-design/web-vue';
import { getToken } from '@/utils/auth';
import $ from 'jquery';
import 'webrtc-adapter';

const route = useRoute();

// 从查询参数获取桌面信息
const desktopName = ref(decodeURIComponent(route.query.name as string || '未知桌面'));
const desktopIP = ref(route.query.ip as string || '未知IP');
const desktopOS = ref(route.query.os as string || '未知系统');
const uid = getToken();
const streamName = desktopIP;
const audio = '1';
const video = '1';

// 拉流相关状态
const remoteVideo = ref<HTMLVideoElement | null>(null);
const pc = ref<RTCPeerConnection | null>(null);
const offer = ref<any>(null);
const remoteStream = ref<MediaStream | null>(null);
const dataChannel = ref<RTCDataChannel | null>(null); // 数据通道
const isActive = ref(false); // 是否激活输入
const videoRect = ref({ left: 0, top: 0, width: 0, height: 0 }); // 视频元素位置和尺寸

// 鼠标事件优化
const lastMousePosition = ref({ x: -1, y: -1 });
const mouseMoveThrottle = ref(0);
const MOUSE_MOVE_THROTTLE = 10; // x毫秒检测一次
const MOUSE_MOVE_THRESHOLD = 0.001; // 相对位置变化阈值

// 更新视频元素位置和尺寸信息
const updateVideoRect = () => {
  if (remoteVideo.value) {
    const rect = remoteVideo.value.getBoundingClientRect();
    videoRect.value = {
      left: rect.left,
      top: rect.top,
      width: rect.width,
      height: rect.height
    };
  }
};

// 调整视频尺寸以保持宽高比
const adjustVideoSize = () => {
  if (!remoteVideo.value) return;
  
  const realtimevideo = remoteVideo.value;
  const container = document.querySelector('.desktop-container') as HTMLElement;
  
  if (!container) return;
  
  const containerWidth = container.clientWidth;
  const containerHeight = container.clientHeight;
  const videoRatio = realtimevideo.videoWidth / realtimevideo.videoHeight;
  const containerRatio = containerWidth / containerHeight;
  
  if (containerRatio > videoRatio) {
    // 容器更宽，以高度为基准
    realtimevideo.style.width = 'auto';
    realtimevideo.style.height = '100%';
  } else {
    // 容器更高，以宽度为基准
    realtimevideo.style.width = '100%';
    realtimevideo.style.height = 'auto';
  }

  // 更新视频元素位置和尺寸信息
  updateVideoRect();
};

// 创建可靠的数据通道
const createDataChannel = () => {
  try {
    // 创建可靠的数据通道
    dataChannel.value = pc.value!.createDataChannel('input', {
      reliable: true,
      // ordered: false, // 是否保证消息顺序
      // maxRetransmits: 0,    // 无重传
    });
    
    dataChannel.value.onopen = () => {
      console.log('数据通道已打开');
      isActive.value = true;
      remoteVideo.value?.focus();
    };
    
    dataChannel.value.onclose = () => {
      console.log('数据通道已关闭');
      isActive.value = false;
    };
  } catch (error) {
    console.error('创建数据通道失败:', error);
  }
};

// 发送输入事件数据
const sendInputEvent = (eventData: any) => {
  if (dataChannel.value && dataChannel.value.readyState === 'open') {
    try {
      const data = JSON.stringify(eventData);
      console.log("准备发送事件：", data)
      dataChannel.value.send(data);
      console.log("事件发送成功：", data)
    } catch (error) {
      console.error('发送输入事件失败:', error);
    }
  }
};

// 计算相对于视频的坐标位置
const getRelativePosition = (event: MouseEvent) => {
  if (!videoRect.value.width || !videoRect.value.height) return { x: 0, y: 0 };
  
  return {
    x: (event.clientX - videoRect.value.left) / videoRect.value.width,
    y: (event.clientY - videoRect.value.top) / videoRect.value.height
  };
};

// 鼠标事件处理
const handleMouseDown = (event: MouseEvent) => {
  if (!isActive.value) return;
  
  const relativePos = getRelativePosition(event);
  lastMousePosition.value = relativePos;

  sendInputEvent({
    type: 'mousedown',
    button: event.button, // 0:左键, 1:中键, 2:右键
    x: relativePos.x,
    y: relativePos.y
  });
};

const handleMouseUp = (event: MouseEvent) => {
  if (!isActive.value) return;
  
  const relativePos = getRelativePosition(event);
  lastMousePosition.value = relativePos;

  sendInputEvent({
    type: 'mouseup',
    button: event.button,
    x: relativePos.x,
    y: relativePos.y
  });
};

// 优化后的鼠标移动事件处理
const handleMouseMove = (event: MouseEvent) => {
  if (!isActive.value) return;
  
  // 节流处理
  const now = Date.now();
  if (now - mouseMoveThrottle.value < MOUSE_MOVE_THROTTLE) {
    return;
  }
  mouseMoveThrottle.value = now;
  
  const relativePos = getRelativePosition(event);
  
  // 检查位置变化是否超过阈值
  const dx = Math.abs(relativePos.x - lastMousePosition.value.x);
  const dy = Math.abs(relativePos.y - lastMousePosition.value.y);
  
  if (dx > MOUSE_MOVE_THRESHOLD || dy > MOUSE_MOVE_THROTTLE) {
    lastMousePosition.value = relativePos;
    
    sendInputEvent({
      type: 'mousemove',
      x: relativePos.x,
      y: relativePos.y
    });
  }
};

const handleContextMenu = (event: MouseEvent) => {
  if (!isActive.value) return;
  event.preventDefault(); // 阻止默认右键菜单
};

// 键盘事件处理
const handleKeyDown = (event: KeyboardEvent) => {
  if (!isActive.value) return;
  
  sendInputEvent({
    type: 'keydown',
    key: event.key,
    code: event.code,
    ctrlKey: event.ctrlKey,
    shiftKey: event.shiftKey,
    altKey: event.altKey,
    metaKey: event.metaKey
  });
};

const handleKeyUp = (event: KeyboardEvent) => {
  if (!isActive.value) return;
  
  sendInputEvent({
    type: 'keyup',
    key: event.key,
    code: event.code,
    ctrlKey: event.ctrlKey,
    shiftKey: event.shiftKey,
    altKey: event.altKey,
    metaKey: event.metaKey
  });
};



// 发送answer
const sendAnswer = (answerSdp: string) => {
  $.post("/signaling/sendanswer", {
    uid,
    streamName: streamName.value,
    answer: answerSdp,
    type: "pull"
  }, () => {}, "json");
};

// 拉流核心逻辑
const pullStream = () => {
  // 创建PeerConnection
  pc.value = new RTCPeerConnection({});
  createDataChannel();
  
  // 处理远程流
  pc.value.ontrack = (event) => {
  const [stream] = event.streams || [];
  
  if (stream) {
    remoteStream.value = stream;
    if (remoteVideo.value) {
      remoteVideo.value.srcObject = stream;

      // 视频加载后调整尺寸
      remoteVideo.value.onloadedmetadata = () => {
          adjustVideoSize();
      };
    }
  }
};
  
  pc.value.setRemoteDescription(offer.value)
    .then(() => {
      // 创建answer
      pc.value.createAnswer()
        .then(answer => {
          pc.value.setLocalDescription(answer)
            .then(() => {
              sendAnswer(answer.sdp);
            });
        });
    });
};

// 开始拉流
const startPull = () => {
  $.post("/signaling/pull", {
    uid,
    streamName: streamName.value,
    audio,
    video
  }, (data, textStatus) => {
    if (textStatus === "success" && data.errNo === 0) {
      offer.value = data.data;
      pullStream();
    }
  }, "json");
};

// 停止拉流
const stopPull = () => {
  if (remoteVideo.value) {
    remoteVideo.value.srcObject = null;
  }
  
  if (remoteStream.value) {
    remoteStream.value.getTracks().forEach(track => track.stop());
    remoteStream.value = null;
  }
  
  if (pc.value) {
    pc.value.close();
    pc.value = null;
  }
  
  $.post("/signaling/stoppull", {
    uid: uid.value,
    streamName: streamName.value
  }, () => {}, "json");
};

// 页面加载时自动开始拉流
onMounted(() => {
  startPull();
  // 窗口大小变化时调整视频尺寸
  window.addEventListener('resize', adjustVideoSize);
});

// 页面卸载前停止拉流
onBeforeUnmount(() => {
  stopPull();
  window.removeEventListener('resize', adjustVideoSize);
});



// 断开连接
const disconnect = () => {
  Message.info('正在断开连接...');
  stopPull();
  setTimeout(() => {
    window.close(); // 关闭当前标签页
    Message.success('已断开连接');
  }, 1000);
};
</script>

<style lang="less" scoped>
.remote-desktop {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: #f5f5f5;
  overflow: hidden; /* 防止滚动条出现 */
  
  .desktop-container {
    flex: 1;
    background-color: #000;
    display: flex;
    justify-content: center;
    align-items: center;
    overflow: hidden; /* 防止滚动条出现 */
    position: relative; /* 为内部元素提供定位上下文 */
    
    .video-wrapper {
      position: relative;
      max-width: 100%;
      max-height: 100%;
      display: flex;
      justify-content: center;
      align-items: center;
      
      .video-element {
        max-width: 100%;
        max-height: 100%;
        object-fit: contain; /* 保持宽高比 */
      }
    }
  }
}

:deep(.arco-page-header) {
  background-color: #fff;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  z-index: 1;
  flex-shrink: 0; /* 防止页头被压缩 */
}
</style>