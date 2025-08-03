#include <absl/base/attributes.h>

#include <rtc_base/logging.h>
#include <rtc_base/synchronization/mutex.h>

#include "pc/srtp_session.h"

namespace xrtc {

SrtpSession::SrtpSession()
{
}

SrtpSession::~SrtpSession() {
    if (_session) {
        srtp_set_user_data(_session, nullptr);
        srtp_dealloc(_session);
    }

    if (_inited) {
        _decrement_libsrtp_usage_count_and_maybe_deinit();
    }
}

bool SrtpSession::set_send(int cs, const uint8_t *key, size_t key_len, const std::vector<int> &extension_ids) {
    return _set_key(ssrc_any_outbound, cs, key, key_len, extension_ids);
}

bool SrtpSession::update_send(int cs, const uint8_t *key, size_t key_len, const std::vector<int> &extension_ids) {
    return _update_key(ssrc_any_outbound, cs, key, key_len, extension_ids);
}

bool SrtpSession::set_recv(int cs, const uint8_t *key, size_t key_len, const std::vector<int> &extension_ids) {
    return _set_key(ssrc_any_inbound, cs, key, key_len, extension_ids);
}

bool SrtpSession::update_recv(int cs, const uint8_t *key, size_t key_len, const std::vector<int> &extension_ids) {
    return _update_key(ssrc_any_inbound, cs, key, key_len, extension_ids);
}

bool SrtpSession::_update_key(int type, int cs, const uint8_t *key, size_t key_len, const std::vector<int> &extension_ids) {
    if (!_session) {
        RTC_LOG(LS_WARNING) << "Failed to update on non-existing session";
        return false;
    }

    return _do_set_key(type, cs, key, key_len, extension_ids);
}

ABSL_CONST_INIT int g_libsrtp_usage_count = 0;
ABSL_CONST_INIT webrtc::GlobalMutex g_libsrtp_lock(absl::kConstInit);

void SrtpSession::_event_handle_thunk(srtp_event_data_t* ev) {
    SrtpSession* session = (SrtpSession*)(srtp_get_user_data(ev->session));
    if (session) {
        session->_handle_event(ev);
    }
}

void SrtpSession::_handle_event(srtp_event_data_t* ev) {
    switch (ev->event) {
        case event_ssrc_collision:
            RTC_LOG(LS_INFO) << "SRTP event: ssrc collision";
            break;
        case event_key_soft_limit:
            RTC_LOG(LS_INFO) << "SRTP event: reached key soft limit";
            break;
        case event_key_hard_limit:
            RTC_LOG(LS_INFO) << "SRTP event: reached key hard limit";
            break;
        case event_packet_index_limit:
            RTC_LOG(LS_INFO) << "SRTP event: packet index limit";
            break;
        default:
            RTC_LOG(LS_WARNING) << "SRTP unknown event: " << ev->event;
            break;
    }
}

bool SrtpSession::_increment_libsrtp_usage_count_and_maybe_init() {
    webrtc::GlobalMutexLock ls(&g_libsrtp_lock);

    if (0 == g_libsrtp_usage_count) {
        int err = srtp_init();
        if (err != srtp_err_status_ok) {
            RTC_LOG(LS_WARNING) << "Failed to init srtp, err: " << err;
            return false;
        }

        err = srtp_install_event_handler(&SrtpSession::_event_handle_thunk);
        if (err != srtp_err_status_ok) {
            RTC_LOG(LS_WARNING) << "Failed to install srtp event, err: " << err;
            return false;
        }

    }

    g_libsrtp_usage_count++;
    return true;
}

void SrtpSession::_decrement_libsrtp_usage_count_and_maybe_deinit() {
    webrtc::GlobalMutexLock ls(&g_libsrtp_lock);
    --g_libsrtp_usage_count;
    if (0 == g_libsrtp_usage_count) {
        int err = srtp_shutdown();
        if (err) {
            RTC_LOG(LS_WARNING) << "Failed to shutdowm, err: " << err;
        }
    }
}

bool SrtpSession::_set_key(int type, int cs, const uint8_t *key, size_t key_len, const std::vector<int> &extension_ids) {
    if (_session) {
        RTC_LOG(LS_WARNING) << "Failed to create session: " << "SRTP session already exists";
        return false;
    }

    if (_increment_libsrtp_usage_count_and_maybe_init()) {
        _inited = true;
    } else {
        return false;
    }

    return _do_set_key(type, cs, key, key_len, extension_ids);
}

bool SrtpSession::_do_set_key(int type, int cs, const uint8_t *key, size_t key_len, const std::vector<int> &extension_ids) {
    srtp_policy_t policy;
    memset(&policy, 0, sizeof(policy));

    bool rtp_ret = srtp_crypto_policy_set_from_profile_for_rtp(&policy.rtp, (srtp_profile_t)cs);
    bool rtcp_ret = srtp_crypto_policy_set_from_profile_for_rtcp(&policy.rtcp, (srtp_profile_t)cs);

    if (rtp_ret != srtp_err_status_ok || rtcp_ret != srtp_err_status_ok) {
        RTC_LOG(LS_WARNING) << "SRTP session " << (_session ? "create" : "update") << " failed: unsupported crypto suite " << cs;
        return false;
    }

    if (!key || key_len != (size_t)policy.rtp.cipher_key_len) {
        RTC_LOG(LS_WARNING) << "SRTP session " << (_session ? "create" : "update") << " failed: invalid key";
        return false;
    }

    policy.ssrc.type = (srtp_ssrc_type_t)type;
    policy.ssrc.value = 0;
    policy.key = (uint8_t*)key;
    policy.window_size = 1024;
    policy.allow_repeat_tx = 1;
    policy.next = nullptr;

    if (!_session) {
        int err = srtp_create(&_session, &policy);
        if (err != srtp_err_status_ok) {
            RTC_LOG(LS_WARNING) << "Failed to create srtp, err: " << err;
            _session = nullptr;
            return false;
        }
        srtp_set_user_data(_session, this);
    } else {
        int err = srtp_update(_session, &policy);
        if (err != srtp_err_status_ok) {
            RTC_LOG(LS_WARNING) << "Failed to update srtp, err: " << err;
            return false;
        }
    }

    _rtp_auth_tag_len = policy.rtp.auth_tag_len;
    _rtcp_auth_tag_len = policy.rtcp.auth_tag_len;
    return true;
}

void SrtpSession::get_auth_tag_len(int* rtp_auth_tag_len, int* rtcp_auth_tag_len) {
    if (!_session) {
        RTC_LOG(LS_WARNING) << "Failed to get auth tag len: no SRTP session";
        return;
    }

    if (rtp_auth_tag_len) {
        *rtp_auth_tag_len = _rtp_auth_tag_len;
    }

    if (rtcp_auth_tag_len) {
        *rtcp_auth_tag_len = _rtcp_auth_tag_len;
    }
}

bool SrtpSession::unprotect_rtp(void* p, int in_len, int* out_len) {
    if (!_session) {
        RTC_LOG(LS_WARNING) << "Failed to unprotect rtp packet: no SRTP session";
        return false;
    }

    uint8_t* srtp_data = static_cast<uint8_t*>(p);
    size_t rtp_len = static_cast<size_t>(in_len);

    // 原地解密（输入输出为同一缓冲区）
    srtp_err_status_t status = srtp_unprotect(
        _session, 
        srtp_data, 
        static_cast<size_t>(in_len),
        srtp_data, // 输出到同一缓冲区
        &rtp_len
    );

    if (status != srtp_err_status_ok) {
        RTC_LOG(LS_WARNING) << "srtp_unprotect failed: " << status;
        return false;
    }
    
    *out_len = static_cast<int>(rtp_len);
    return true;
}

bool SrtpSession::unprotect_rtcp(void* p, int in_len, int* out_len) {
    if (!_session) {
        RTC_LOG(LS_WARNING) << "Failed to unprotect rtcp packet: no SRTP session";
        return false;
    }

    uint8_t* srtp_data = static_cast<uint8_t*>(p);
    size_t rtp_len = static_cast<size_t>(in_len);

    // 原地解密（输入输出为同一缓冲区）
    srtp_err_status_t status = srtp_unprotect_rtcp(
        _session, 
        srtp_data, 
        static_cast<size_t>(in_len),
        srtp_data, // 输出到同一缓冲区
        &rtp_len
    );

    if (status != srtp_err_status_ok) {
        RTC_LOG(LS_WARNING) << "srtp_unprotect failed: " 
                           << status;
        return false;
    }
    
    *out_len = static_cast<int>(rtp_len);
    return true;
}

bool SrtpSession::protect_rtp(void* p, int in_len, int max_len, int* out_len){
    if (!_session) {
        RTC_LOG(LS_WARNING) << "Failed to protect rtp packet: no SRTP session";
        return false;
    }

    // 计算所需的最小缓冲区大小
    size_t min_required_len = static_cast<size_t>(in_len) + _rtp_auth_tag_len;
    if (static_cast<size_t>(max_len) < min_required_len) {
        RTC_LOG(LS_WARNING) << "Failed to protect rtp packet: The buffer length " 
                           << max_len << " is less than needed " << min_required_len;
        return false;
    }

    // 准备调用参数
    uint8_t* rtp_data = static_cast<uint8_t*>(p);
    size_t srtp_len = static_cast<size_t>(max_len);
    
    // 调用库函数
    srtp_err_status_t err = srtp_protect(
        _session,         // SRTP 会话上下文
        rtp_data,         // 输入 RTP 数据
        static_cast<size_t>(in_len), // 输入数据长度
        rtp_data,         // 输出 SRTP 数据（原地操作）
        &srtp_len,        // 输入输出参数：输入为最大长度，输出为实际长度
        0                 // MKI 索引（通常为0）
    );

    if (err != srtp_err_status_ok) {
        RTC_LOG(LS_WARNING) << "Failed to protect rtp packet: err=" << err;
        return false;
    }

    *out_len = static_cast<int>(srtp_len);
    return true;
}
    
bool SrtpSession::protect_rtcp(void* p, int in_len, int max_len, int* out_len){
    if (!_session) {
        RTC_LOG(LS_WARNING) << "Failed to protect rtcp packet: no SRTP session";
        return false;
    }

    // 计算所需的最小缓冲区大小
    size_t min_required_len = static_cast<size_t>(in_len) + _rtcp_auth_tag_len + sizeof(uint32_t);
    if (static_cast<size_t>(max_len) < min_required_len) {
        RTC_LOG(LS_WARNING) << "Failed to protect rtcp packet: The buffer length " 
                           << max_len << " is less than needed " << min_required_len;
        return false;
    }

    // 准备调用参数
    uint8_t* rtcp_data = static_cast<uint8_t*>(p);
    size_t srtp_len = static_cast<size_t>(max_len);
    
    // 调用库函数
    srtp_err_status_t err = srtp_protect_rtcp(
        _session,         // SRTP 会话上下文
        rtcp_data,         // 输入 RTCP 数据
        static_cast<size_t>(in_len), // 输入数据长度
        rtcp_data,         // 输出 SRTP 数据（原地操作）
        &srtp_len,        // 输入输出参数：输入为最大长度，输出为实际长度
        0                 // MKI 索引（通常为0）
    );

    if (err != srtp_err_status_ok) {
        RTC_LOG(LS_WARNING) << "Failed to protect rtcp packet: err=" << err;
        return false;
    }

    *out_len = static_cast<int>(srtp_len);
    return true;
}

}