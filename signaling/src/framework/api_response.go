package framework

// ApiResponse 用于前后端通信的统一响应结构
type ApiResponse struct {
	Data   interface{} `json:"data"`   // 实际数据
	Status string      `json:"status"` // 状态字符串: "ok" 或 "fail"
	Msg    string      `json:"msg"`    // 消息
	Code   int         `json:"code"`   // 状态码
}

type UserInfo struct {
	Name            string `json:"name"`
	Avatar          string `json:"avatar"`
	Email           string `json:"email"`
	Job             string `json:"job"`
	JobName         string `json:"jobName"`
	Organization    string `json:"organization"`
	OrganizationName string `json:"organizationName"`
	Location        string `json:"location"`
	LocationName    string `json:"locationName"`
	Introduction    string `json:"introduction"`
	PersonalWebsite string `json:"personalWebsite"`
	Phone           string `json:"phone"`
	RegistrationDate string `json:"registrationDate"`
	AccountId       string `json:"accountId"`
	Certification   int    `json:"certification"`
	Role            string `json:"role"`
}

// ApiSuccess 创建成功响应
func ApiSuccess(data interface{}) ApiResponse {
	return ApiResponse{
		Data:   data,
		Status: "ok",
		Msg:    "请求成功",
		Code:   20000,
	}
}

// ApiFail 创建失败响应
func ApiFail(msg string, code int, data interface{}) ApiResponse {
	return ApiResponse{
		Data:   data,
		Status: "fail",
		Msg:    msg,
		Code:   code,
	}
}

// ApiValidationError 创建验证错误响应
func ApiValidationError(errors interface{}) ApiResponse {
	return ApiResponse{
		Data:   errors,
		Status: "fail",
		Msg:    "验证失败",
		Code:   40001,
	}
}

// ApiServerError 创建服务器错误响应
func ApiServerError(msg string) ApiResponse {
	return ApiResponse{
		Data:   nil,
		Status: "fail",
		Msg:    msg,
		Code:   50000,
	}
}