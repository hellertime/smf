// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
// std
#include <chrono>
#include <iostream>
// linux
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
// seastar
#include <core/app-template.hh>
#include <core/distributed.hh>
#include <core/sleep.hh>
#include <net/api.hh>
// smf
#include "integration_tests/non_root_port.h"
#include "smf/log.h"
#include "smf/random.h"
#include "smf/rpc_generated.h"
#include "smf/rpc_handle_router.h"
#include "smf/rpc_recv_context.h"
#include "smf/rpc_server.h"
// templates
#include "integration_tests/demo_service.smf.fb.h"

// For a network connected host we should always have at least an lo and eth0 interface 
// (or whatever the connected interface is called) ... so capping the maximum test clients
// at 2 will cover this. Going larger doesn't improve the accuracy of this test.
constexpr static const size_t kMaxClients = 2;

static inline
std::string ipv4_addr_to_ip_str(seastar::ipv4_addr addr) {
    std::stringstream addr_;
    addr_ << addr;
    std::string ip = addr_.str().substr(0, addr_.str().find(":"));
    return std::move(ip);
}

struct client_info {
    explicit client_info(seastar::ipv4_addr ip): client(smf_gen::demo::SmfStorageClient::make_shared(ip)), ip(ip.ip) {};
    client_info(client_info &&o) noexcept : client(std::move(o.client)), ip(o.ip) {};
    ~client_info(){};
    seastar::shared_ptr<smf_gen::demo::SmfStorageClient> client = nullptr;
    uint32_t ip = 0;
    SMF_DISALLOW_COPY_AND_ASSIGN(client_info);
};

struct ifaddrs_t {
    ifaddrs_t() { LOG_THROW_IF(getifaddrs(&ifaddr_) == -1, "failed to getifaddrs()"); }
    ~ifaddrs_t() { freeifaddrs(ifaddr_); }
    struct ifaddrs *ifaddr_ = nullptr;
};

class clients {
public:
    explicit clients(uint16_t port) {
        auto ifaddr = ifaddrs_t();
        struct ifaddrs *ifa = clients::next_valid_interface(ifaddr.ifaddr_);
        for (int i = 0; i < kMaxClients; ifa = ifa->ifa_next, i++) {
            ifa = clients::next_valid_interface(ifa);
            THROW_IFNULL(ifa);
            auto sa_in = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            auto ip = ntohl(sa_in->sin_addr.s_addr);
            clients_.push_back(client_info(seastar::make_ipv4_address(ip, port)));
            DLOG_DEBUG("Added client {}", clients_[i].client->server_addr);
        }
    }

    auto begin() { return clients_.begin(); }
    auto end() { return clients_.end(); }
private:
    static struct ifaddrs* next_valid_interface(struct ifaddrs* ifa) {
        while (ifa && !(ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_flags & IFF_RUNNING)) {
            ifa = ifa->ifa_next;
        }
        DLOG_DEBUG("Next valid interface {}", ifa->ifa_name);
        return ifa;
    }
    std::vector<struct client_info> clients_;
};

class storage_service : public smf_gen::demo::SmfStorage {
  virtual seastar::future<smf::rpc_typed_envelope<smf_gen::demo::Response>>
  Get(smf::rpc_recv_typed_context<smf_gen::demo::Request> &&rec) final {
    smf::rpc_typed_envelope<smf_gen::demo::Response> data;
    if (rec) {
        DLOG_DEBUG("Got request name {}", rec->name()->str());
        data.data->name = ipv4_addr_to_ip_str(rec.ctx->remote_address);
    }
    data.envelope.set_status(200);
    return seastar::make_ready_future<
        smf::rpc_typed_envelope<smf_gen::demo::Response>>(std::move(data));
  }
};

int
main(int args, char **argv, char **env) {
  DLOG_DEBUG("About to start the RPC test");
  seastar::distributed<smf::rpc_server> rpc;

  seastar::app_template app;

  smf::random rand;
  uint16_t random_port =
    smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());

  return app.run(args, argv, [&]() -> seastar::future<int> {
    DLOG_DEBUG("Setting up at_exit hooks");
    seastar::engine().at_exit([&] { return rpc.stop(); });

    smf::random r;
    smf::rpc_server_args sargs;
    sargs.ip = "0.0.0.0";
    sargs.rpc_port = random_port;
    sargs.http_port =
      smf::non_root_port(rand.next() % std::numeric_limits<uint16_t>::max());
    sargs.flags |= smf::rpc_server_flags::rpc_server_flags_disable_http_server;
    return rpc.start(sargs)
      .then([&rpc] {
        return rpc.invoke_on_all(
          &smf::rpc_server::register_service<storage_service>);
      })
      .then([&rpc] {
        DLOG_DEBUG("Invoking rpc start on all cores");
        return rpc.invoke_on_all(&smf::rpc_server::start);
      })
      .then([random_port] {
        return seastar::do_with(clients(random_port), [] (auto& clients) {
          return seastar::do_for_each(clients, [] (auto& client) {
            DLOG_DEBUG("Connecting client to {}", client.client->server_addr);
            return client.client->connect()
            .then([&client] {
              DLOG_DEBUG("Connected to {}", client.client->server_addr);
              smf::rpc_typed_envelope<smf_gen::demo::Request> req;
              std::stringstream addr;
              addr << client.client->server_addr;
              req.data->name = std::move(addr.str());
              return client.client->Get(req.serialize_data());
            })
            .then([&client] (auto reply) {
              DLOG_DEBUG("Got reply {}", reply->name()->str());
              if (reply->name()->str() != ipv4_addr_to_ip_str(client.client->server_addr)) {
                  LOG_THROW("Server did not see our IP {} != {}", reply->name()->str(), client.client->server_addr);
              }
              return seastar::make_ready_future<>();
            })
            .then([&client] { 
              return client.client->stop(); 
            });
          });
        });
      })
      .then([] {
        DLOG_DEBUG("Exiting");
        return seastar::make_ready_future<int>(0);
      });
  });
}
