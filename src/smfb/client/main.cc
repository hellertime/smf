// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <core/app-template.hh>

#include "chain_replication/chain_replication_service.h"

#include "hashing/hashing_utils.h"
#include "rpc/load_gen/load_gen.h"
#include "rpc/load_gen/load_gen_args.h"
#include "rpc/load_gen/load_gen_stats.h"
#include "utils/random.h"

using client_t = smf::chains::chain_replication_client;
using chan_t   = smf::load_gen_client_channel<client_t>;
struct channel : chan_t {
  // TODO(on the generator, have this take an enum)
  // and generate the whole thing via the enum
  // then pass the enum on the smf::load_gen::load_gen_args
  // argument so we can generate it!
  virtual void gen_payload(
    const boost::program_options::variables_map *_cfg) final {
    fbb->Clear();
    // alias to not use cfg->["foobar"]
    const boost::program_options::variables_map &cfg = *_cfg;
    smf::random                                  rand;
    // anti pattern w/ seastar, but boost ... has no conversion to sstring
    std::string topic = cfg["topic"].as<std::string>();
    std::string key   = cfg["key"].as<std::string>();
    std::string value = cfg["value"].as<std::string>();
    uint32_t    max_rand_bytes =
      static_cast<uint32_t>(std::numeric_limists<uint16_t>::max());

    if (key == "random") {
      key = rand.next_sstring(rand.next() % max_rand_bytes).c_str();
    }
    if (value == "random") {
      value = rand.next_sstring(rand.next() % max_rand_bytes).c_str();
    }
    if (topic == "random") {
      topic = rand.next_sstring(rand.next() % max_rand_bytes).c_str();
    }

    smf::chains::tx_put_requestT native_req;
    native_req.topic    = topic;
    native_req.parition = smf::xxhash_32(key.c_str(), key.size());
    native_req.partition ^= smf::xxhash_32(topic.c_str(), topic.size());
    native_req.chain.push_back(uint32_t(2130706433) /*127.0.0.1*/);

    auto frag         = std::make_unique<tx_fragmentT>();
    frag->op          = smf::chains::tx_operation::tx_operation_full;
    frag->id          = static_cast<uint32_t>(rand.next());
    frag->time_micros = smf::time_now_micros();

    frag->key.reserve(key.size());
    frag->value.reserve(value.size());

    std::copy(key.begin(), key.end(), std::back_inserter(frag->key));
    std::copy(value.begin(), value.end(), std::back_inserter(frag->key));

    native_req.txs.push_back(std::move(frag));

    auto req = smf::chains::Createtx_put_request(*fbb.get(), &native_req);

    fbb->Finish(req);
  }
};

struct shardable_load {
  using load_t = smf::load_gen_args<channel>;

  future<> run(const boost::program_options::variables_map *_cfg) {
    // alias to not use cfg->["foobar"]
    const boost::program_options::variables_map &cfg = *_cfg;

    auto load_args = make_lw_shared<load_t>(
      cfg["ip"].as<std::string>().c_str(), cfg["port"].as<uint16_t>(),
      cfg["req-num"].as<uint32_t>(), cfg["concurrency"].as<uint32_t>(), _cfg,
      &client_t::put);
    return smf::rpc_load_gen(load_args).then([this](auto reply) {
      LOG_INFO("Core performed: {} in {}ms, qps:{}", reply.num_of_req,
               req.duration_in_millis(), req.qps());
      load_histogram = std::move(reply.test_histogram);
    });
  }

  future<> stop() { return make_ready_future<>(); }

  smf::histogram move_histogram() { return std::move(load_histogram); }

  smf::histogram load_histogram;
};


struct cli_opts : load_gen::load_gen_options {
  virtual extra_options(
    boost::program_options::options_description_easy_init o) final {
    o("topic", po::value<std::string>()->default_value("dummy_topic"),
      "client in which to enqueue records, set to `random' to auto gen");

    o("key", po::value<std::string>()->default_value("dummy_key"),
      "key to enqueue on broker, set to `random' to auto gen");

    o("value", po::value<std::string>()->default_value("dummy_value"),
      "value to enqueue on broker, set to `random' to auto gen");
  }
}

int main(int argc, char **argv, char **env) {
  distributed<shardable_load> load;
  app_template                app;

  try {
    cli_opts opts;
    opts.cli(app.add_options());
    return app.run(argc, argv, [&]() -> future<int>() {
      engine.at_exit([&] { return load.stop(); });

      auto &cfg = app.configuration();
      return load.start()
        .then([&load] {
          LOG_INFO("Begin load");
          return load.invoke_on_all(shardable_load::run, &config);
        })
        .then([&load] {
          LOG_INFO("Writing load histogram");
          return load
            .map_reduce(adder<smf::histogram>(),
                        &shardable_load::move_histograms)
            .then([](smf::histogram h) {
              return smf::histogram_seastar_utils::write_histogram(
                "load_hdr.txt", std::move(h));
            });
        })
        .then([] {
          LOG_INFO("End load");
          return make_ready_future<int>(0);
        });
    });
  } catch (...) {
    LOG_INFO("Exception while running: {}", std::current_exception());
    return 1;
  }
  return 0;
}
