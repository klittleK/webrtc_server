#include <rtc_base/logging.h>

#include "base/socket.h"
#include "base/async_udp_socket.h"
#include "async_udp_socket.h"


namespace xrtc { 

const size_t MAX_BUF_SIZE = 1500;


UdpPacketData::UdpPacketData(const char *data, size_t size, const rtc::SocketAddress &addr)
    : _data(new char[size]),
    _size(size),
    _addr(addr)
{
    memcpy(_data, data, size);
}

UdpPacketData::~UdpPacketData() {
    if (_data) {
        delete []_data;
        _data = nullptr;
    }
}

void async_udp_socket_io_cb(EventLoop* el, IOWatcher* w, int fd, int events, void* data) {
    AsyncUdpSocket* udp_socket = (AsyncUdpSocket*)data;
    if (EventLoop::READ & events) {
        udp_socket->recv_data();
    }

    if (EventLoop::WRITE & events) {
        udp_socket->send_data();
    }
}

AsyncUdpSocket::AsyncUdpSocket(EventLoop *el, int socket) :
    _el(el),
    _socket(socket),
    _buf(new char[MAX_BUF_SIZE]),
    _size(MAX_BUF_SIZE)
{
    _socket_watcher = _el->create_io_event(async_udp_socket_io_cb, this);
    _el->start_io_event(_socket_watcher, _socket, EventLoop::READ);
}

AsyncUdpSocket::~AsyncUdpSocket() {
    if (_socket_watcher) {
        _el->delete_io_event(_socket_watcher);
        _socket_watcher = nullptr;
    }

    if (_buf) {
        delete []_buf;
        _buf = nullptr;
    }
}

void AsyncUdpSocket::recv_data() {
    while (true) {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        int len = sock_recv_from(_socket, _buf, _size, (struct sockaddr*)&addr, addr_len);
        if (len <= 0) {
            return;
        }

        int64_t ts = sock_get_recv_timestamp(_socket);
        int port = ntohs(addr.sin_port);
        char ip[64] = {0};
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        rtc::SocketAddress remote_addr(ip, port);

        signal_read_packet(this, _buf, len, remote_addr, ts);
    }
}

void xrtc::AsyncUdpSocket::send_data() {
    size_t len = 0;
    int sent = 0;
    while (!_udp_packet_list.empty()) {
        // 从队列中取出一个UDP包进行发送
        UdpPacketData* packet = _udp_packet_list.front();
        sockaddr_storage saddr;
        len = packet->addr().ToSockAddrStorage(&saddr);
        sent = sock_send_to(_socket, packet->data(), packet->size(), MSG_NOSIGNAL, (struct sockaddr*)&saddr, len);
        if (sent < 0) {
            RTC_LOG(LS_WARNING) << "send udp packet error, remote_addr: " << packet->addr().ToString();
            delete packet;  // 发送失败，删除该包
            _udp_packet_list.pop_front();
            return;
        } else if (sent == 0) {
            RTC_LOG(LS_WARNING) << "send 0 bytes, try again, remote_addr: " << packet->addr().ToString();
            return;  // 发送了0字节，可能是连接关闭或其他错误
        } else {
            delete packet;  // 发送成功，删除该包
            _udp_packet_list.pop_front();
        }
    }

    if (_udp_packet_list.empty()) {
        // 如果队列为空，停止写事件
        _el->stop_io_event(_socket_watcher, _socket, EventLoop::WRITE);
    }
}

int AsyncUdpSocket::send_to(const char* data, size_t size, const rtc::SocketAddress& addr){
    return _add_udp_packet(data, size, addr);
}

int AsyncUdpSocket::_add_udp_packet(const char* data, size_t size, const rtc::SocketAddress& addr) {
    UdpPacketData* udp_packet = new UdpPacketData(data, size, addr);
    _udp_packet_list.push_back(udp_packet);
    _el->start_io_event(_socket_watcher, _socket, EventLoop::WRITE);

    return size;  // 返回发送的字节数
}

}