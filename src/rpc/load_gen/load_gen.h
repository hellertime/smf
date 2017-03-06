#pragma once

#include <algorithm>
#include <memory>

#include <core/shared_ptr.hh>

#include "rpc/load_gen/load_gen_args.h"
#include "rpc/load_gen/load_gen_stats.h"

namespace smf {
namespace load_gen {

template <typename Service>
future<load_gen_stats> rpc_load_gen(lw_shared_ptr<load_gen_args> args) {
  return do_for_each(args->clients.begin(), args->clients.end(),
                     [](auto &c) { return c->connect(); })
    .then([args = ptr] {
      namespace co   = std::chrono;
      auto begin     = co::high_resolution_clock::now();
      auto launch_fn = [args](auto &limit) {
        return do_for_each(
                 args->clients.begin(), args->clients.end(),
                 [&limit, &args](auto &c) {
                   return limit.wait(1).then([this, &c, &limit, &args] {
                     // notice that this does not return, hence
                     // executing concurrently
                     auto req = args->num_of_req / args->concurrency;
                     c->invoke(req, args->fn).finally([&limit] {
                       limit.signal(1);
                     });
                   });
                 })
          .then([args, &limit] {
            // now let's wait for ALL to finish
            return limit.wait(args->concurrency);
          });
      };
      return do_with(semaphore(args->concurrency), launch_fn)
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
      auto req = args->num_of_req / args->concurrency;
      return make_ready_future<load_gen_stats>(req, duration, std::move(h));
    });
}

}  // namespace load_gen
}  // namespace smf
