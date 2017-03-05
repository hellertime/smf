// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <core/app-template.hh>

#include "chain_replication/chain_replication_service.h"

void add_cli_opts(boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("port", po::value<uint16_t>()->default_value(11201), "rpc port");

  o("topic", po::value<std::string>()->default_value("smfb_client_topic"),
    "client in which to enqueue records");

  o("key", po::value<std::string>()->default_value("dummy_key"),
    "key to enqueue on broker");

  o("value", po::value<std::string>()->default_value("dummy_value"),
    "value to enqueue on broker");

  o("req-num", po::value<uint32_t>()->default_value(1000),
    "number of requests");

  o("concurrency", po::value<uint32_t>()->default_value(10),
    "number of concurrent requests, per core");

  o("broker-ip", po::value<std::string>()->default_value("127.0.0.1"),
    "ip address of server");
}


int main(int argc, char **argv, char **env) {
  // distributed<smf::rpc_server> rpc;


  // app_template app;

  // try {
  //   add_cli_opts(app.add_options());

  //   return app.run_deprecated(argc, argv, [&] {
  //     LOG_INFO("Setting up at_exit hooks");
  //     engine().at_exit([&] {});
  //   });
  // } catch (...) {
  //   LOG_INFO("Exception while running broker: {}", std::current_exception());
  //   return 1;
  // }
  return 0;
}
