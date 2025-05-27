#include <ifaddrs.h>
#include <netinet/in.h>
#include <memory>

#include <rtc_base/logging.h>

#include "network.h"

namespace xrtc {

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager() {
    for (auto network : _network_list) {
        delete network;
        network = nullptr;
    }

    _network_list.clear();
}

// 获取公网 IP 的函数
std::string get_public_ip_from_command() {
    std::string result;

    // 使用 popen 执行系统命令
    FILE* pipe = popen("curl -s ifconfig.me", "r");
    if (!pipe) {
        RTC_LOG(LS_WARNING) << "Failed to execute command";
        return "";
    }

    // 读取命令的输出
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    // 关闭管道
    pclose(pipe);

    // 去除换行符
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

int NetworkManager::create_networks() {
    struct ifaddrs* interface;
    int err = getifaddrs(&interface);
    if (err != 0) {
        RTC_LOG(LS_WARNING) << "getifaddrs error: " << strerror(errno) << ", errno: " << errno;
        return -1;
    }

    for (auto cur = interface; cur != nullptr; cur = cur->ifa_next) {
        if (cur->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        struct sockaddr_in* addr = (struct sockaddr_in*)(cur->ifa_addr);
        rtc::IPAddress ip_address(addr->sin_addr);

        /*if (rtc::IPIsPrivateNetwork(ip_address) || rtc::IPIsLoopback(ip_address)) {
            continue;
        }*/

        if (rtc::IPIsLoopback(ip_address)) {
            continue;
        }

        Network* network = new Network(cur->ifa_name, ip_address);

        RTC_LOG(LS_INFO) << "gathered network interface: " << network->to_string();

        _network_list.push_back(network);
    }

    freeifaddrs(interface);

    // 获取公网 IP 并添加到列表中
    std::string public_ip_str = get_public_ip_from_command();
    if (!public_ip_str.empty()) {
        rtc::IPAddress public_ip;
        if (IPFromString(public_ip_str, &public_ip)) {
            Network* public_network = new Network("public", public_ip);
            RTC_LOG(LS_INFO) << "gathered public IP: " << public_network->to_string();
            _network_list.push_back(public_network);
        } else {
            RTC_LOG(LS_WARNING) << "Failed to parse public IP: " << public_ip_str;
        }
    } else {
        RTC_LOG(LS_WARNING) << "Failed to retrieve public IP";
    }

    return 0;
}

}