#pragma once

#include <algorithm>
#include <memory>

#include <core/shared_ptr.hh>

#include "rpc/load_gen_client_channel.h"
#include "histogram/histogram.h"

namespace smf {

template <typename Service> struct load_gen_args {
  using client_t = std::unique_ptr<load_gen_client_channel<Service>>;

  load_gen_args(const char *_ip,
                uint16_t    _port,
                size_t      _num_of_req,
                size_t      _concurrency,
                auto (Service::*_func)(smf::rpc_envelope),
                const char *    _payload,
                size_t          _payload_size)
    : ip(_ip)
    , port(_port)
    , num_of_req(_num_of_req)
    , concurrency(_concurrency)
    , fn(_func)
    , payload(_payload)
    , payload_size(_payload_size)
    , clients(_concurrency) {
    std::generate(clients.begin(), clients.end(), [&args] {
      // populate ALL clients
      return std::make_unique<channel_t>(args.ip, args.port);
    });
  }

  const char *ip;
  uint16_t    port;
  size_t      num_of_req;
  size_t      concurrency;

  decltype(std::mem_fn(auto (Service::*func)(smf::rpc_envelope))) fn;

  const char *                            payload;
  size_t                                  payload_size;
  std::vector<std::unique_ptr<channel_t>> clients;
};

struct load_gen_duration {
  load_gen_duration(uint64_t reqs, std::chrono::duration _d, smf::histogram &&h)
    : num_of_req(reqs), test_duration(_d), test_histogram(std::move(h)) {}

  uint64_t              num_of_req;
  std::chrono::duration test_duration;
  smf::histogram        test_histogram;

  uint64_t duration_in_millis() {
    namespace co = std::chrono;
    return co::duration_cast<co::milliseconds>(duration.count());
  }

  uint64_t qps() {
    auto       milli = duration_in_millis();
    const auto reqs  = static_cast<double>(num_of_req);

    // some times the test run under 1 millisecond
    const auto safe_denom = std::max<uint64_t>(milli, 1);
    const auto denom      = static_cast<double>(safe_denom);

    auto queries_per_milli = reqs / denom;

    return queries_per_milli * 1000.0;
  }
};

template <typename Service>
future<std::chrono::duration, smf::histogram> rpc_load_gen(
  lw_shared_ptr<load_gen_args> args) {
  return do_for_each(args->clients.begin(), args->clients.end(),
                     [](auto &c) { return c->connect(); })
    .then([args = ptr] {
      namespace co = std::chrono;
      auto begin   = co::high_resolution_clock::now();
      return do_with(
               semaphore(args->concurrency),
               [&args](auto &limit) {
                 return do_for_each(
                          args->clients.begin(), args->clients.end(),
                          [&limit, &args](auto &c) {
                            return limit.wait(1).then(
                              [this, &c, &limit, &args] {
                                // notice that this does not return, hence
                                // executing concurrently
                                auto req = args->num_of_req / args->concurrency;
                                c->invoke(req, args->fn, args->payload,
                                          args->payload_size)
                                  .finally([&limit] { limit.signal(1); });
                              });
                          })
                   .then([this, &limit] { return limit.wait(concurrency_); });
               })
        .then([args, begin] {
          auto                  end_time = co::high_resolution_clock::now();
          std::chrono::duration d        = (end_time - begin);
          return make_ready_future<std::chrono::duration>(d);
        });
    })
    .then([args](auto duration) {
      smf::histogram h;
      for (auto &ch : clients_) {
        h += ch->get_histogram();
      }
      return make_ready_future<load_gen_duration>(args->num_of_req, duration,
                                                  std::move(h));
    });
}

}  // namespace smf
