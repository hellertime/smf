#pragma once

#include "platform/log.h"
#include "platform/macros.h"

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
    : client(new smf_gen::fbs::rpc::SmfStorageClient(ipv4_addr{ip, port}))
    , fbb(new flatbuffers::FlatBufferBuilder()) {
    client->enable_histogram_metrics();
    client->register_outgoing_filter(smf::zstd_compression_filter(1000));
  }
  load_gen_client_channel(load_gen_client_channel &&o)
    : client(std::move(o.client)), fbb(std::move(o.fbb)) {}

  SMF_DISALLOW_COPY_AND_ASSIGN(load_gen_client_channel);


  histogram *get_histogram() const { return client->get_histogram(); }

  future<> connect() { return client->connect(); }

  /// template <typename Args... args>
  /// ((*inst).*func)(args...);
  /// Note that we'll need to do this when we type the sends
  ///
  future<> invoke(uint32_t reqs, Service::*func) {
    LOG_THROW_IF(reqs == 0, "bad number of requests");
    LOG_THROW_IF(fbb->GetSize() == 0, "Empty builder, don't forget to build "
                                      "flatbuffers payload. See the "
                                      "documentation.");

    return do_for_each(
      boost::counting_iterator<uint32_t>(0),
      boost::counting_iterator<uint32_t>(reqs), [this, func](int) {
        smf::rpc_envelope e(fbb->GetBufferPointer(), fbb->GetSize());
        return ((*client).*func)(std::move(e)).then([](auto t) {
          return make_ready_future<>();
        });
      });
  }

  std::unique_ptr<Service>                        client;
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb;
};
}
