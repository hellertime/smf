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

  inline future<> invoke(uint32_t reqs,
                         auto (Service::*func)(smf::rpc_envelope),
                         const char *    payload,
                         size_t          payload_size) {
    LOG_THROW_IF(reqs == 0, "bad number of requests");
    LOG_THROW_IF(fbb->GetSize() == 0, "Empty builder, don't forget to build "
                                      "flatbuffers payload. See the "
                                      "documentation.");

    return do_for_each(
      boost::counting_iterator<uint32_t>(0),
      boost::counting_iterator<uint32_t>(reqs),
      [this, func, payload, payload_size](uint32_t i) mutable {
        smf::rpc_envelope e(payload, payload_size);
        return ((*client).*func)(std::move(e)).then([](auto t) {
          return make_ready_future<>();
        });
      });
  }

  SMF_DISALLOW_COPY_AND_ASSIGN(load_gen_client_channel);
  std::unique_ptr<Service>                        client;
  std::unique_ptr<flatbuffers::FlatBufferBuilder> fbb;
};
}
