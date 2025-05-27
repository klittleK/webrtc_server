package main

import (
	"fmt"
	"net/http"
)

func startHttp(port string) {
	fmt.Printf("Start http port: %s\n", port)
	err := http.ListenAndServe(port, nil)
	if err != nil {
		fmt.Println(err)
	}
}

func startHttps(port, cert, key string) {
	fmt.Printf("Start https port: %s\n", port)
	err := http.ListenAndServeTLS(port, cert, key, nil)
	if err != nil {
		fmt.Println(err)
	}
}

func main() {
	// url前缀
	staticUrl := "/static/"
	fs := http.FileServer(http.Dir("./static"))

	// 绑定url和fileserver
	http.Handle(staticUrl, http.StripPrefix(staticUrl, fs))

	// 协程启动http server
	go startHttp(":8080")
	// 启动https server 
	startHttps(":8081", "./conf/fullchain.pem", "./conf/privkey.pem")
}
