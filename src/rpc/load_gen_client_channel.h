#pragma once

#include "platform/log.h"
#include "platform/macros.h"
#include "rpc/filters/zstd_filter.h"
#include "rpc/rpc_envelope.h"

namespace smf {


/// \brief used to send N requests, correctly. one at a time and enqueue
/// callbacks for read - i.e.: future<> to parse in order
///
/// Don't forget to build the builder before sending requests
/// \code{.cpp}
///        auto req = smf_gen::fbs::rpc::CreateRequest(
///        *fbb.get(), fbb->CreateString(...));
///        ...
///        fbb->Finish(req);
/// \endcode
///
template <typename Service> struct load_gen_client_channel {
  load_gen_client_channel(const char *ip, uint16_t port)
    : client(new Service(ipv4_addr{ip, port}))
    , fbb(new flatbuffers::FlatBufferBuilder()) {
    client->enable_histogram_metrics();
    // client->register_outgoing_filter<smf::zstd_compression_filter>(1000);
  }

  load_gen_client_channel(load_gen_client_channel &&o)
    : client(std::move(o.client)), fbb(std::move(o.fbb)) {}

  virtual ~load_gen_client_channel() {}

  virtual histogram *get_histogram() const { return client->get_histogram(); }

  virtual future<> connect() { return client->connect(); }

  /// template <typename Args... args>
  /// ((*inst).*func)(args...);
  /// Note that we'll need to do this when we type the sends
  ///
  template <typename... Args>
  // typename Ret = typename std::result_of<decltype(
  //   auto (Service::*func)(Args...))>::type>
  inline future<> invoke(uint32_t reqs,
                         auto (Service::*func)(Args...),
                         Args &&... args) {
    LOG_THROW_IF(reqs == 0, "bad number of requests");
    LOG_THROW_IF(fbb->GetSize() == 0, "Empty builder, don't forget to build "
                                      "flatbuffers payload. See the "
                                      "documentation.");
    auto tuple_args = std::make_tuple<Args...>(args...);

    return do_for_each(
      boost::counting_iterator<uint32_t>(0),
      boost::counting_iterator<uint32_t>(reqs),
      [this, targs = std::move(tuple_args), func](uint32_t i) mutable {
        auto payload = gen_args<smf::rpc_envelope>(
          std::index_sequence_for<Args...>(), std::move(targs));

        return ((*client).*func)(std::move(payload)).then([](auto t) {
          return make_ready_future<>();
        });
      });
  }

  template <typename T, size_t... Indices, typename... Args>
  auto gen_args(std::index_sequence<Indices...>, std::tuple<Args...> args) {
    return T(std::get<Indices>(args)...);
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(load_gen_client_channel);
  std::unique_ptr<Service>                        client;
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb;
};
}
