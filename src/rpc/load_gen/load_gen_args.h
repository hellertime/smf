#pragma once

#include <memory>
#include <vector>

#include "platform/log.h"
#include "rpc/load_gen/load_gen_client_channel.h"

namespace smf {
namespace load_gen {
// XXX(agallego) - this needs to be refactored when we enable *always*
// streaming


template <typename Service> struct load_gen_args {
  using client_t = std::unique_ptr<load_gen_client_channel<Service>>;

  load_gen_args(const char *                                 _ip,
                uint16_t                                     _port,
                size_t                                       _num_of_req,
                size_t                                       _concurrency,
                const boost::program_options::variables_map *_cfg,
                auto (Service::*_func)(smf::rpc_envelope))
    : ip(_ip)
    , port(_port)
    , num_of_req(_num_of_req)
    , concurrency(_concurrency)
    , fn(_func)
    , cfg(THROW_IFNULL(_cfg))
    , clients(_concurrency) {
    std::generate(clients.begin(), clients.end(), [&args] {
      // populate ALL clients
      auto client = std::make_unique<channel_t>(args.ip, args.port);
      client->gen_payload();
      return std::move(client);
    });
  }

  const char *ip;
  uint16_t    port;
  size_t      num_of_req;
  size_t      concurrency;

  const boost::program_options::variables_map *cfg;

  decltype(std::mem_fn(auto (Service::*func)(smf::rpc_envelope))) fn;

  // You can only have one client per active stream
  // the problem comes when you try to read, 2 function calls to read, even
  // read_exactly might interpolate
  // which yields incorrect results for test. That use case has to be manually
  // optimized and don't expect it to be the main use case
  // In effect, you need a socket per concurrency item in the command line
  // flags
  //
  std::vector<std::unique_ptr<channel_t>> clients;
};

}  // namespace load_gen
}  // namespace smf
