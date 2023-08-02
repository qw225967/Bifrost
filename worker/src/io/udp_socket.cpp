/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 1:39 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : udp_socket.cpp
 * @description : TODO
 *******************************************************/

#include "udp_socket.h"

#include <iostream>

#include "utils.h"

namespace bifrost {
/* Static. */

static constexpr size_t ReadBufferSize{65536};
static uint8_t ReadBuffer[ReadBufferSize];

/* Static methods for UV callbacks. */

inline static void onAlloc(uv_handle_t* handle, size_t suggested_size,
                           uv_buf_t* buf) {
  auto* socket = static_cast<UdpSocket*>(handle->data);

  if (socket) socket->OnUvRecvAlloc(suggested_size, buf);
}

inline static void onRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
                          const struct sockaddr* addr, unsigned int flags) {
  auto* socket = static_cast<UdpSocket*>(handle->data);

  if (socket) socket->OnUvRecv(nread, buf, addr, flags);
}

inline static void onSend(uv_udp_send_t* req, int status) {
  auto* sendData = static_cast<UdpSocket::UvSendData*>(req->data);
  auto* handle = req->handle;
  auto* socket = static_cast<UdpSocket*>(handle->data);
  auto* cb = sendData->cb;

  if (socket) socket->OnUvSend(status, cb);

  // Delete the UvSendData struct (it will delete the store and cb too).
  delete sendData;
}

inline static void onClose(uv_handle_t* handle) { delete handle; }

/* Instance methods. */

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
UdpSocket::UdpSocket(uv_udp_t* uv_handle_) : uv_handle_(uv_handle_) {
  int err;

  this->uv_handle_->data = static_cast<void*>(this);

  err = uv_udp_recv_start(this->uv_handle_, static_cast<uv_alloc_cb>(onAlloc),
                          static_cast<uv_udp_recv_cb>(onRecv));

  if (err != 0) {
    uv_close(reinterpret_cast<uv_handle_t*>(this->uv_handle_),
             static_cast<uv_close_cb>(onClose));

    std::cout << "[udp_socket] uv_udp_recv_start failed: " << uv_strerror(err)
              << std::endl;
  }

  // Set local address.
  if (!SetLocalAddress()) {
    uv_close(reinterpret_cast<uv_handle_t*>(this->uv_handle_),
             static_cast<uv_close_cb>(onClose));

    std::cout << "[udp_socket] error setting local IP and port" << std::endl;
  }
}

UdpSocket::~UdpSocket() {
  if (!this->closed_) Close();
}

void UdpSocket::Close() {
  if (this->closed_) return;

  this->closed_ = true;

  // Tell the UV handle that the UdpSocket has been closed_.
  this->uv_handle_->data = nullptr;

  // Don't read more.
  int err = uv_udp_recv_stop(this->uv_handle_);

  if (err != 0)
    std::cout << "[udp_socket] uv_udp_recv_stop failed: " << uv_strerror(err)
              << std::endl;

  uv_close(reinterpret_cast<uv_handle_t*>(this->uv_handle_),
           static_cast<uv_close_cb>(onClose));
}

void UdpSocket::Dump() const {
  std::cout << "[udp_socket] <UdpSocket>" << std::endl;
  std::cout << "[udp_socket]   localIp   : " << this->local_ip_.c_str()
            << std::endl;
  std::cout << "[udp_socket]   localPort : "
            << static_cast<uint16_t>(this->local_port_) << std::endl;
  std::cout << "[udp_socket]   closed_    : "
            << (!this->closed_ ? "open" : "closed_") << std::endl;
  std::cout << "[udp_socket] </UdpSocket>" << std::endl;
}

void UdpSocket::Send(const uint8_t* data, size_t len,
                     const struct sockaddr* addr,
                     UdpSocket::onSendCallback* cb) {
  if (this->closed_) {
    if (cb) (*cb)(false);

    return;
  }

  if (len == 0) {
    if (cb) (*cb)(false);

    return;
  }

  // First try uv_udp_try_send(). In case it can not directly send the datagram
  // then build a uv_req_t and use uv_udp_send().

  uv_buf_t buffer =
      uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len);
  int sent = uv_udp_try_send(this->uv_handle_, &buffer, 1, addr);

  // Entire datagram was sent. Done.
  if (sent == static_cast<int>(len)) {
    // Update sent bytes.
    this->sent_bytes_ += sent;

    if (cb) {
      (*cb)(true);

      delete cb;
    }

    return;
  } else if (sent >= 0) {
    //    std::cout << "[udp_socket] datagram truncated just " << sent << " of
    //    bytes "
    //              << len << " were sent" << std::endl;

    // Update sent bytes.
    this->sent_bytes_ += sent;

    if (cb) {
      (*cb)(false);

      delete cb;
    }

    return;
  }
  // Any error but legit EAGAIN. Use uv_udp_send().
  else if (sent != UV_EAGAIN) {
    std::cout << "[udp_socket] uv_udp_try_send failed, trying uv_udp_send(): "
              << uv_strerror(sent) << std::endl;
    return;
  }

  auto* sendData = new UvSendData(len);

  sendData->req.data = static_cast<void*>(sendData);
  std::memcpy(sendData->store, data, len);
  sendData->cb = cb;

  buffer = uv_buf_init(reinterpret_cast<char*>(sendData->store), len);

  int err = uv_udp_send(&sendData->req, this->uv_handle_, &buffer, 1, addr,
                        static_cast<uv_udp_send_cb>(onSend));

  if (err != 0) {
    // NOTE: uv_udp_send() returns error if a wrong INET family is given
    // (IPv6 destination on a IPv4 binded socket), so be ready.
    std::cout << "[udp_socket] uv_udp_send failed: " << uv_strerror(err)
              << std::endl;

    if (cb) (*cb)(false);

    // Delete the UvSendData struct (it will delete the store and cb too).
    delete sendData;
  } else {
    // Update sent bytes.
    this->sent_bytes_ += len;
  }
}

bool UdpSocket::SetLocalAddress() {
  int err;
  int len = sizeof(this->local_addr_);

  err = uv_udp_getsockname(
      this->uv_handle_, reinterpret_cast<struct sockaddr*>(&this->local_addr_),
      &len);

  if (err != 0) {
    std::cout << "[udp_socket] uv_udp_getsockname failed: " << uv_strerror(err)
              << std::endl;

    return false;
  }

  int family;

  IP::get_address_info(
      reinterpret_cast<const struct sockaddr*>(&this->local_addr_), family,
      this->local_ip_, this->local_port_);

  return true;
}

inline void UdpSocket::OnUvRecvAlloc(size_t /*suggested_size*/, uv_buf_t* buf) {
  // Tell UV to write into the static buffer.
  buf->base = reinterpret_cast<char*>(ReadBuffer);
  // Give UV all the buffer space.
  buf->len = ReadBufferSize;
}

inline void UdpSocket::OnUvRecv(ssize_t nread, const uv_buf_t* buf,
                                const struct sockaddr* addr,
                                unsigned int flags) {
  // NOTE: Ignore if there is nothing to read or if it was an empty datagram.
  if (nread == 0) return;

  // Check flags.
  if ((flags & UV_UDP_PARTIAL) != 0u) {
    std::cout << "[udp_socket] received datagram was truncated due to "
                 "insufficient buffer, ignoring "
                 "it"
              << std::endl;

    return;
  }

  // Data received.
  if (nread > 0) {
    // Update received bytes.
    this->recv_bytes_ += nread;

    // Notify the subclass.
    UserOnUdpDatagramReceived(reinterpret_cast<uint8_t*>(buf->base), nread,
                              addr);
  }
  // Some error.
  else {
    std::cout << "[udp_socket] read error: " << uv_strerror(nread) << std::endl;
  }
}

inline void UdpSocket::OnUvSend(int status, UdpSocket::onSendCallback* cb) {
  if (status == 0) {
    if (cb) (*cb)(true);
  } else {
#if MS_LOG_DEV_LEVEL == 3
    MS_DEBUG_DEV("send error: %s", uv_strerror(status));
#endif

    if (cb) (*cb)(false);
  }
}
}  // namespace bifrost