// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TCP_BOUND_SOCKET_H_
#define SERVICES_NETWORK_TCP_BOUND_SOCKET_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/socket/tcp_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"
#include "services/network/tcp_server_socket.h"

namespace net {
class IPEndPoint;
class NetLog;
}  // namespace net

namespace network {
class SocketFactory;

// A socket bound to an address. Can be converted into either a TCPServerSocket
// or a TCPConnectedSocket.
class COMPONENT_EXPORT(NETWORK_SERVICE) TCPBoundSocket
    : public mojom::TCPBoundSocket {
 public:
  // Constructs a TCPBoundSocket. |socket_factory| must outlive |this|. When a
  // new connection is accepted, |socket_factory| will be notified to take
  // ownership of the connection.
  TCPBoundSocket(SocketFactory* socket_factory,
                 net::NetLog* net_log,
                 const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~TCPBoundSocket() override;

  // Attempts to bind a socket to the specified address. Returns net::OK on
  // success, setting |local_addr_out| to the bound address. Returns a network
  // error code on failure. Must be called before Listen() or Connect().
  int Bind(const net::IPEndPoint& local_addr, net::IPEndPoint* local_addr_out);

  // Sets the id used to remove the socket from the owning BindingSet. Must be
  // called before Listen() or Connect().
  void set_id(mojo::BindingId binding_id) { binding_id_ = binding_id; }

  // mojom::TCPBoundSocket implementation.
  void Listen(uint32_t backlog,
              mojom::TCPServerSocketRequest request,
              ListenCallback callback) override;
  void Connect(const net::IPEndPoint& remote_addr,
               mojom::TCPConnectedSocketRequest request,
               mojom::SocketObserverPtr observer,
               ConnectCallback callback) override;

 private:
  void OnConnectComplete(int result);

  mojo::BindingId binding_id_ = -1;
  SocketFactory* const socket_factory_;
  std::unique_ptr<net::TCPSocket> socket_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  mojom::TCPConnectedSocketRequest connected_socket_request_;
  mojom::SocketObserverPtr socket_observer_;
  ConnectCallback connect_callback_;

  base::WeakPtrFactory<TCPBoundSocket> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TCPBoundSocket);
};

}  // namespace network

#endif  // SERVICES_NETWORK_TCP_BOUND_SOCKET_H_
