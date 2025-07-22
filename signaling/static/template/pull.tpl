<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>kk云桌面</title>
    <style type="text/css">
        #debugInfo {
            height: 200px;
            width: 600px;
            overflow-y: auto;
            border: 1px solid #ccc;
            padding: 10px;
            background: #333;
            color: #fff;
            font-family: monospace;
            margin-top: 15px;
            border-radius: 4px;
        }
        
        #debugInfo .entry {
            margin-bottom: 5px;
            padding-bottom: 5px;
            border-bottom: 1px solid #555;
        }
        
        #debugInfo .time {
            color: #0f0;
            margin-right: 10px;
        }
        
        #debugInfo .message {
            color: #fff;
        }

        /* 其他样式保持不变 */
        body { font-size: 13px; }
        .highlight {
            background-color: #eeeeee;
            margin: 0 0 5px 0;
            padding: 0.5em 1.5em;
        }
        video {
            width: 480px;
            height: 360px;
        }
        button {
            background-color: #d84a38;
            border: none;
            border-radius: 2px;
            color: white;
            margin: 5px 0 0 0;
            padding: 0.5em 0.7em 0.6em 0.7em;
        }
        button:hover {
            background-color: #cf402f;
        }
    </style>
</head>
<body>
    <h3>拉流端</h3>
    <div class="highlight">
        拉流端基本信息
        <span>
            uid={{.uid}}
            streamName={{.streamName}}
            audio={{.audio}}
            video={{.video}}
        </span>
    </div>
    <span id="tips1"></span> <br>
    <span id="tips2"></span> <br>
    <span id="tips3"></span>
    <div style="margin-top:5px">
        <video id="remoteVideo" controls autoplay></video>
    </div>
    <button id="pullBtn">开始拉流</button>
    <button id="stopPullBtn">停止拉流</button>
    <input type="hidden" id="uid" value="{{.uid}}"/>
    <input type="hidden" id="streamName" value="{{.streamName}}"/>
    <input type="hidden" id="audio" value="{{.audio}}"/>
    <input type="hidden" id="video" value="{{.video}}"/>
    
    <div id="debugInfo"></div>
    
    <script src="/static/js/adapter.js"></script>
    <script src="/static/js/jquery-2.1.1.min.js"></script>
    <script src="/static/js/pull.js"></script>
</body>
</html>