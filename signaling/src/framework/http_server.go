package framework

import (
	"fmt"
	"net/http"
	"os"
	"path/filepath"
	"signaling/src/glog"
	"strconv"
	"strings"
)

// 自启动的入口
func init() {
	http.HandleFunc("/", entry)
}

// 对请求的处理方法接口
type ActionInterface interface {
	// 真正的处理方法
	Execute(w http.ResponseWriter, cr *ComRequest)
}

// url-处理方法的映射
var GActionRouter map[string]ActionInterface = make(map[string]ActionInterface)

type ComRequest struct {
	R      *http.Request
	Logger *ComLog
	LogId  uint32
}

func responseError(w http.ResponseWriter, r *http.Request, status int, err string) {
	w.WriteHeader(status)
	w.Write([]byte(fmt.Sprintf("%d - %s", status, err)))
}

func getRealClientIP(r *http.Request) string {
	ip := r.RemoteAddr

	if rip := r.Header.Get("X-Real-IP"); rip != "" {
		ip = rip
	} else if rip = r.Header.Get("X-Forwarded-IP"); rip != "" {
		ip = rip
	}

	return ip
}

func entry(w http.ResponseWriter, r *http.Request) {
	fmt.Println("==========", r.URL.Path)
	// 找处理方法
	if action, ok := GActionRouter[r.URL.Path]; ok {
		if action != nil {
			cr := &ComRequest{
				R:      r,
				Logger: &ComLog{},
				LogId:  GetLogId32(),
			}

			cr.Logger.AddNotice("logId", strconv.Itoa(int(cr.LogId)))
			cr.Logger.AddNotice("url", r.URL.Path)
			cr.Logger.AddNotice("referer", r.Header.Get("Referer"))
			cr.Logger.AddNotice("cookie", r.Header.Get("Cookie"))
			cr.Logger.AddNotice("ua", r.Header.Get("User-Agent"))
			cr.Logger.AddNotice("clientIP", r.RemoteAddr)
			cr.Logger.AddNotice("realClientIP", getRealClientIP(r))

			r.ParseForm()

			for k, v := range r.Form {
				cr.Logger.AddNotice(k, v[0])
			}

			cr.Logger.TimeBegin("totalCost")
			action.Execute(w, cr)
			cr.Logger.TimeEnd("totalCost")

			cr.Logger.Infof("")
		} else {
			responseError(w, r, http.StatusInternalServerError, "Internal server error")
		}
		return
	}

	// 处理静态资源请求
	if strings.HasPrefix(r.URL.Path, gconf.httpStaticPrefix) {
		// 获取相对路径
		relativePath := strings.TrimPrefix(r.URL.Path, "/")
		fmt.Println("相对路径", relativePath)
		// 构建完整文件路径
		filePath := filepath.Join(gconf.httpStaticDir, relativePath)
		fmt.Println("完整文件路径", filePath)

		// 检查文件是否存在
		if _, err := os.Stat(filePath); !os.IsNotExist(err) {
			fmt.Println("文件存在")
			// 设置正确的 MIME 类型
			if strings.HasSuffix(filePath, ".js") {
				w.Header().Set("Content-Type", "application/javascript")
			} else if strings.HasSuffix(filePath, ".css") {
				w.Header().Set("Content-Type", "text/css")
			}

			http.ServeFile(w, r, filePath)
			return
		}
	}

	// 所有其他请求返回 Vue 应用入口
	indexPath := filepath.Join(gconf.httpStaticDir, "index.html")
	http.ServeFile(w, r, indexPath)
}

func RegisterStaticUrl() {
	fs := http.FileServer(http.Dir(gconf.httpStaticDir))
	http.Handle(gconf.httpStaticPrefix, http.StripPrefix(gconf.httpStaticPrefix, fs))
}

// 启动监听
func StartHttp() error {
	glog.Infof("start http server on port: %d", gconf.httpPort)
	return http.ListenAndServe(fmt.Sprintf(":%d", gconf.httpPort), nil)
}

func StartHttps() error {
	glog.Infof("start https server on port: %d", gconf.httpsPort)
	return http.ListenAndServeTLS(fmt.Sprintf(":%d", gconf.httpsPort),
		gconf.httpsCert, gconf.httpsKey, nil)
}
