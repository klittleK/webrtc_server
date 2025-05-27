package action

import (
	"encoding/json"
	"fmt"
	"net/http"
	"signaling/src/comerrors"
	"signaling/src/framework"
	"strconv"
)

type sendAnswerAction struct{}

func NewSendAnswerAction() *sendAnswerAction {
	return &sendAnswerAction{}
}

type xrtcSendAnswerReq struct {
	Cmdno      int    `json:"cmdno"`
	Uid        uint64 `json:"uid"`
	StreamName string `json:"stream_name"`
	Answer     string `json:"answer"`
	Type       string `json:"type"`
}

type xrtcSendAnswerResp struct {
	Errno  int    `json:"err_no"`
	ErrMsg string `json:"err_msg"`
}

func (*sendAnswerAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
	r := cr.R

	// uid
	var strUid string
	if values, ok := r.Form["uid"]; ok {
		strUid = values[0]
	}

	uid, err := strconv.ParseUint(strUid, 10, 64)
	if err != nil || uid <= 0 {
		cerr := comerrors.New(comerrors.ParamErr, "parse uid error: "+err.Error())
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	// streamName
	var streamName string
	if values, ok := r.Form["streamName"]; ok {
		streamName = values[0]
	}

	if "" == streamName {
		cerr := comerrors.New(comerrors.ParamErr, "streamName is null")
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	// answer
	var answer string
	if values, ok := r.Form["answer"]; ok {
		answer = values[0]
	}

	if "" == answer {
		cerr := comerrors.New(comerrors.ParamErr, "answer is null")
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	// type
	var strType string
	if values, ok := r.Form["type"]; ok {
		strType = values[0]
	}

	if "" == strType {
		cerr := comerrors.New(comerrors.ParamErr, "strType is null")
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	req := xrtcSendAnswerReq{
		Cmdno:      CMDNO_ANSWER,
		Uid:        uid,
		StreamName: streamName,
		Answer:     answer,
		Type:       strType,
	}

	var resp xrtcSendAnswerResp

	err = framework.Call("xrtc", req, &resp, cr.LogId)
	if err != nil {
		cerr := comerrors.New(comerrors.NetworkErr, "backend process error: "+err.Error())
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	if resp.Errno != 0 {
		cerr := comerrors.New(comerrors.NetworkErr,
			fmt.Sprintf("backend process error: %d", resp.Errno))
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	// 构造http响应
	httpResp := comHttpResp{
		ErrNo:  0,
		ErrMsg: "success",
	}

	b, _ := json.Marshal(httpResp)
	cr.Logger.AddNotice("resp", string(b))
	w.Write(b)
}
