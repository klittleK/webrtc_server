package action

import (
	"encoding/json"
	"net/http"
	"signaling/src/framework"
	"strings"
)

// UserInfoAction 处理用户信息请求
type UserInfoAction struct{}

// NewUserInfoAction 创建新的 UserInfoAction 实例
func NewUserInfoAction() *UserInfoAction {
	return &UserInfoAction{}
}

// Execute 执行用户信息请求
func (a *UserInfoAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
	r := cr.R
	logger := cr.Logger

	// 添加 CORS 支持
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Authorization, Content-Type")

	// 处理预检请求
	if r.Method == "OPTIONS" {
		w.WriteHeader(http.StatusOK)
		return
	}

	// 确保设置正确的 Content-Type
	w.Header().Set("Content-Type", "application/json")

	// 获取授权头
	authHeader := r.Header.Get("Authorization")
	if authHeader == "" {
		logger.Infof("未提供授权头")
		w.WriteHeader(http.StatusUnauthorized)
		json.NewEncoder(w).Encode(framework.ApiFail("未登录", 50008, nil))
		return
	}

	// 解析 token (Bearer <token>)
	parts := strings.Split(authHeader, " ")
	if len(parts) != 2 || parts[0] != "Bearer" {
		logger.Infof("无效的授权格式")
		w.WriteHeader(http.StatusUnauthorized)
		json.NewEncoder(w).Encode(framework.ApiFail("无效的授权格式", 50008, nil))
		return
	}

	token := parts[1]

	// 验证 token
	if !isValidToken(token) {
		logger.Infof("无效的 token: %s", token)
		w.WriteHeader(http.StatusUnauthorized)
		json.NewEncoder(w).Encode(framework.ApiFail("无效的 token", 50008, nil))
		return
	}

	// 获取用户角色 (根据 token 确定)
	role := getUserRole(token)

	// 构建用户信息
	userInfo := framework.UserInfo{
		Name:             "admin",
		Avatar:           "",
		Email:            "1551085149@qq.com",
		Job:              "student",
		JobName:          "kaibo",
		Organization:     "EDU",
		OrganizationName: "USTC",
		Location:         "中国",
		LocationName:     "合肥",
		Introduction:     "。。。",
		PersonalWebsite:  "https://lkbstream.cn",
		Phone:            "13245139441",
		RegistrationDate: "2025-07-31 12:10:00",
		AccountId:        "1234567890",
		Certification:    1,
		Role:             role,
	}

	// 返回成功响应
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(framework.ApiSuccess(userInfo))
	logger.Infof("返回用户信息: role=%s", role)
}

// 验证 token 有效性
func isValidToken(token string) bool {
	// 实际项目中应使用 JWT 验证
	// 这里简化验证
	return token == "8888" || token == "9999"
}

// 根据 token 获取用户角色
func getUserRole(token string) string {
	// 实际项目中应从 token 解析
	// 这里简化处理
	if token == "9999" {
		return "admin"
	}
	return "user"
}
