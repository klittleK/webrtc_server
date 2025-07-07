'use strict'

const debugInfo = document.getElementById("debugInfo");
var localVideo = document.getElementById("localVideo")
var pushBtn = document.getElementById("pushBtn");

pushBtn.addEventListener("click", startPush);

var uid = $("#uid").val();
var streamName = $("#streamName").val();
var audio = $("#audio").val();
var video = $("#video").val();
var offer = "";
var pc;
const config = {};
var localStream;
var lastConnectionState = "";

function addDebugInfo(message) {
    console.log(`[DEBUG] ${message}`);
    
    if (!debugInfo) return;
    
    // 创建带时间戳的调试条目
    const timestamp = new Date().toLocaleTimeString();
    const debugEntry = document.createElement("div");
    debugEntry.className = "debug-entry";
    
    const timeSpan = document.createElement("span");
    timeSpan.className = "debug-time";
    timeSpan.textContent = `[${timestamp}]`;
    
    const messageSpan = document.createElement("span");
    messageSpan.className = "debug-msg";
    messageSpan.textContent = message;
    
    debugEntry.appendChild(timeSpan);
    debugEntry.appendChild(messageSpan);
    
    // 添加到调试信息区域
    debugInfo.appendChild(debugEntry);
    
    // 自动滚动到底部保持可见
    debugInfo.scrollTop = debugInfo.scrollHeight;
}

function logTrackStatus() {
    if (!localStream) return;
    
    const videoTracks = localStream.getVideoTracks();
    const audioTracks = localStream.getAudioTracks();
    
    addDebugInfo(`轨道状态: 视频=${videoTracks.length} 音频=${audioTracks.length}`);
    
    if (videoTracks.length > 0) {
        const track = videoTracks[0];
        addDebugInfo(`视频轨道状态: readyState=${track.readyState}, enabled=${track.enabled}, muted=${track.muted}`);
    }
}

async function logStats() {
    if (!pc) return;
    
    try {
        const stats = await pc.getStats();
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
            addDebugInfo(`视频统计: 包=${videoStats.packetsSent}, 字节=${videoStats.bytesSent}`);
        } else {
            addDebugInfo('未找到视频发送统计');
        }
        
        if (audioStats) {
            addDebugInfo(`音频统计: 包=${audioStats.packetsSent}, 字节=${audioStats.bytesSent}`);
        }
    } catch (error) {
        addDebugInfo(`获取统计失败: ${error.message}`);
    }
}

function startPush() {
    console.log("send push: /signaling/push");
    addDebugInfo("发起推流请求");

    $.post("/signaling/push",
        {"uid": uid, "streamName": streamName, "audio": audio, "video": video},
        function(data, textStatus) {
            console.log("push response: " + JSON.stringify(data));
            if("success" == textStatus && 0 == data.errNo) {
                $("#tips1").html("<font color='blue'>推流请求成功！</font>");
                console.log("remote offer: \r\n" + data.data.sdp);
                offer = data.data;
                
                // 调试信息 - 检查offer SDP
                const hasVideo = data.data.sdp.includes("m=video");
                addDebugInfo(`推流请求成功，SDP包含视频: ${hasVideo}`);
                
                pushStream();
            } else {
                $("#tips1").html("<font color='blue'>推流请求失败！</font>");
                addDebugInfo("推流请求失败");
            }
        },
        "json"
    );
}

function sendAnswer(answerSdp) {
    console.log("send answer: /signaling/sendanswer");
    
    // 调试信息 - 检查answer SDP
    const hasVideo = answerSdp.includes("m=video");
    addDebugInfo(`发送answer，SDP包含视频: ${hasVideo}`);

    $.post("/signaling/sendanswer",
        {"uid": uid, "streamName": streamName, "answer": answerSdp, "type": "push"},
        function(data, textStatus) {
            console.log("send answer response: " + JSON.stringify(data));
            if("success" == textStatus && 0 == data.errNo) {
                $("#tips3").html("<font color='blue'>answer发送成功！</font>");
                addDebugInfo("answer发送成功");
                
                // 定期检查状态
                setInterval(() => {
                    logTrackStatus();
                    logStats();
                }, 3000);
            } else {
                $("#tips3").html("<font color='blue'>answer发送失败！</font>");
                addDebugInfo("answer发送失败");
            }
        },
        "json"
    );
}

function pushStream() {
    console.log("创建PeerConnection");
    pc = new RTCPeerConnection(config);

    // 添加ICE候选项收集监听
    pc.onicecandidate = function(event) {
        if (event.candidate) {
            addDebugInfo("新增ICE候选项: " + event.candidate.candidate);
        }
    };
    
    pc.oniceconnectionstatechange = function(e) {
        var state = "";
        if (lastConnectionState != "") {
            state = lastConnectionState + "->" + pc.iceConnectionState;
        } else {
            state = pc.iceConnectionState;
        }
        
        $("#tips2").html("连接状态： " + state);
        lastConnectionState = pc.iceConnectionState;
        
        // 调试信息
        addDebugInfo(`ICE连接状态变化: ${state}`);
        
        // 连接成功时记录状态
        if (pc.iceConnectionState === "connected") {
            addDebugInfo("ICE连接已建立");
            logTrackStatus();
        }
    }
    
    pc.setRemoteDescription(offer).then(
        setRemoteDescriptionSuccess,
        setRemoteDescriptionError,
    );
}

window.addEventListener("message", function(event) {
    if (event.origin != window.location.origin) {
        return;
    }

    if (event.data.type) {
        if (event.data.type == "SS_DIALOG_SUCCESS") {
            console.log("用户同意屏幕共享， streamId: " + event.data.streamId);
            addDebugInfo(`用户同意屏幕共享，streamId: ${event.data.streamId}`);
            startScreenStreamFrom(event.data.streamId);
        } else if (event.data.type == "SS_DIALOG_CANCEL") {
            console.log("用户取消屏幕共享");
            addDebugInfo("用户取消屏幕共享");
        }
    }
});

function startScreenStreamFrom(streamId) {
    addDebugInfo(`开始屏幕共享，streamId: ${streamId}`);
    
    var constraints = {
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

    navigator.mediaDevices.getUserMedia(constraints).then(
        handleSuccess).catch(handleError);
}

function handleSuccess(stream) {
    addDebugInfo("获取屏幕共享流成功");
    
    navigator.mediaDevices.getUserMedia({audio: true}).then(
        function(audioStream) {
            addDebugInfo("获取音频流成功");
            
            // 记录轨道状态
            logTrackStatus();
            
            stream.addTrack(audioStream.getAudioTracks()[0]);
            localVideo.srcObject = stream;
            localStream = stream;
            
            addDebugInfo("将流添加到PeerConnection");
            pc.addStream(stream);
            
            addDebugInfo("开始创建answer");
            pc.createAnswer().then(
                createSessionDescriptionSuccess,
                createSessionDescriptionError
            );
        }
    ).catch(handleError)
}

function handleError(error) {
    console.log("get user media error: " + error);
    addDebugInfo(`获取用户媒体错误: ${error.message}`);
}

function setRemoteDescriptionSuccess() {
    console.log("pc set remote description success");
    console.log("request screen share");
    addDebugInfo("远端点描述设置成功");
    
    // 检查SDP包含视频信息
    if (offer.sdp) {
        const hasVideo = offer.sdp.includes("m=video");
        addDebugInfo(`远端点描述包含视频: ${hasVideo}`);
    }
    
    window.postMessage({type: "SS_UI_REQUEST", text: "push"}, "*");
}

function setRemoteDescriptionError(error) {
    console.log("pc set remote description error: " + error);
    addDebugInfo(`远端点描述设置错误: ${error.message}`);
}

function createSessionDescriptionSuccess(answer) {
    console.log("answer sdp: \n" + answer.sdp);
    console.log("pc set local sdp");
    
    // 检查answer SDP
    if (answer.sdp) {
        const hasVideo = answer.sdp.includes("m=video");
        addDebugInfo(`answer包含视频: ${hasVideo}`);
    }
    
    pc.setLocalDescription(answer).then(
        setLocalDescriptionSuccess,
        setLocalDescriptionError
    );

    sendAnswer(answer.sdp);
}

function createSessionDescriptionError(error) {
    console.log("pc create answer error: " + error);
    addDebugInfo(`创建answer错误: ${error.message}`);
}

function setLocalDescriptionSuccess() {
    console.log("set local description success");
    addDebugInfo("本地描述设置成功");
}

function setLocalDescriptionError(error) {
    console.log("pc set local description error: " + error);
    addDebugInfo(`本地描述设置错误: ${error.message}`);
}