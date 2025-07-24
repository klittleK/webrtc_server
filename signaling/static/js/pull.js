'use strict'

const debugInfo = document.getElementById("debugInfo");
var remoteVideo = document.getElementById("remoteVideo")
var pullBtn = document.getElementById("pullBtn");
var stopPullBtn = document.getElementById("stopPullBtn");

pullBtn.addEventListener("click", startPull);
stopPullBtn.addEventListener("click", stopPull);

var uid = $("#uid").val();
var streamName = $("#streamName").val();
var audio = $("#audio").val();
var video = $("#video").val();
var offer = "";
var pc;
const config = {};
var remoteStream;
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

async function logStats() {
    if (!pc) return;
    
    try {
        const stats = await pc.getStats();
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
            addDebugInfo(`视频统计: 包=${videoStats.packetsReceived}, 字节=${videoStats.bytesReceived}`);
        } else {
            addDebugInfo('未找到视频发送统计');
        }
        
        if (audioStats) {
            addDebugInfo(`音频统计: 包=${audioStats.packetsReceived}, 字节=${audioStats.bytesReceived}`);
        }
    } catch (error) {
        addDebugInfo(`获取统计失败: ${error.message}`);
    }
}

function startPull() {
    console.log("send pull: /signaling/pull");
    addDebugInfo("发起拉流请求");

    $.post("/signaling/pull",
        {"uid": uid, "streamName": streamName, "audio": audio, "video": video},
        function(data, textStatus) {
            console.log("pull response: " + JSON.stringify(data));
            if("success" == textStatus && 0 == data.errNo) {
                $("#tips1").html("<font color='blue'>拉流请求成功！</font>");
                console.log("remote offer: \r\n" + data.data.sdp);
                offer = data.data;
                
                // 调试信息 - 检查offer SDP
                const hasVideo = data.data.sdp.includes("m=video");
                addDebugInfo(`拉流请求成功，SDP包含视频: ${hasVideo}`);
                
                pullStream();
            } else {
                $("#tips1").html("<font color='blue'>拉流请求失败！</font>");
                addDebugInfo("拉流请求失败");
            }
        },
        "json"
    );
}

function stopPull() {
    console.log("send stop pull: /signaling/stoppull");

    remoteVideo.srcObject = null;
    if (remoteStream && localStream.getAudioTracks()) {
        remoteStream.getAudioTracks()[0].stop();
    }

    if (remoteStream && localStream.getVideoTracks()) {
        remoteStream.getVideoTracks()[0].stop();
    }

    if (pc) {
        pc.close();
        pc = null;
    }

    $("#tips1").html("");
    $("#tips2").html("");
    $("#tips3").html("");

    $.post("/signaling/stoppull",
        {"uid": uid, "streamName": streamName},
        function(data, textStatus) {
            console.log("stop pull response: " + JSON.stringify(data));
            if("success" == textStatus && 0 == data.errNo) {
                $("#tips1").html("<font color='blue'>停止拉流请求成功！</font>");
            } else {
                $("#tips1").html("<font color='blue'>停止拉流请求失败！</font>");
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
        {"uid": uid, "streamName": streamName, "answer": answerSdp, "type": "pull"},
        function(data, textStatus) {
            console.log("send answer response: " + JSON.stringify(data));
            if("success" == textStatus && 0 == data.errNo) {
                $("#tips3").html("<font color='blue'>answer发送成功！</font>");
                addDebugInfo("answer发送成功");
                
                // 定期检查状态
                setInterval(() => {
                    //logStats();
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

function pullStream() {
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
            logStats();
        }
    }

    pc.onaddstream = function(e) {
        remoteStream = e.stream;
        remoteVideo.srcObject = e.stream;
    }

    console.log("set remote sdp start");
    
    pc.setRemoteDescription(offer).then(
        setRemoteDescriptionSuccess,
        setRemoteDescriptionError,
    );
}

function setRemoteDescriptionSuccess() {
    console.log("pc set remote sdp success");
    pc.createAnswer().then(
        createSessionDescriptionSuccess,
        createSessionDescriptionError
    );
}

function setRemoteDescriptionError(error) {
    console.log("pc set remote sdp error: " + error);
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
    console.log("set local sdp success");
    addDebugInfo("本地描述设置成功");
}

function setLocalDescriptionError(error) {
    console.log("pc set local sdp error: " + error);
    addDebugInfo(`本地描述设置错误: ${error.message}`);
}