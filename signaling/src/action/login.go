package action

import (
	"encoding/json"
	"net/http"
	"signaling/src/framework"
)

// LoginAction 处理用户登录
type LoginAction struct{}

// NewLoginAction 创建新的 LoginAction 实例
func NewLoginAction() *LoginAction {
	return &LoginAction{}
}

// 登录请求结构体
type loginRequest struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

// 登录成功数据
type loginSuccessData struct {
	Token string `json:"token"`
}

// Execute 执行登录操作
func (a *LoginAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
	r := cr.R
	logger := cr.Logger

	// 添加 CORS 支持
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

	// 处理预检请求
	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	// 确保设置正确的 Content-Type
	w.Header().Set("Content-Type", "application/json")

	// 解析请求体
	var req loginRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		logger.Infof("解析请求体失败: %v", err)
		w.WriteHeader(http.StatusBadRequest)
		json.NewEncoder(w).Encode(framework.ApiFail("Invalid request body", 50001, nil))
		return
	}

	logger.Infof("登录请求: username=%s", req.Username)

	// 简单验证用户名和密码
	if req.Username == "admin" && req.Password == "admin" {
		// 返回成功响应
		successData := loginSuccessData{
			Token: "9999",
		}

		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(framework.ApiSuccess(successData))
		logger.Infof("登录成功: username=%s", req.Username)
	} else if req.Username == "user" && req.Password == "user" {
		// 返回成功响应
		successData := loginSuccessData{
			Token: "8888",
		}

		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(framework.ApiSuccess(successData))
		logger.Infof("登录成功: username=%s", req.Username)
	} else {
		// 验证失败
		logger.Infof("登录失败: 无效凭证")
		w.WriteHeader(http.StatusUnauthorized)
		json.NewEncoder(w).Encode(framework.ApiFail("Invalid credentials", 50001, nil))
	}
}
