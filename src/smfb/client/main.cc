// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#include <iostream>

#include <core/app-template.hh>

#include "chain_replication/chain_replication_service.h"

void add_cli_opts(
  boost::program_options::options_description_easy_init o) {
  namespace po = boost::program_options;

  o("port", po::value<uint16_t>()->default_value(11201), "rpc port");

  o("key", po::value<uint16_t>()->default_value(11201), "rpc port");

  o("rpc-stats-period-mins", po::value<uint32_t>()->default_value(5),
    "period to print stats in minutes");

  o("wal-stats-period-mins", po::value<uint32_t>()->default_value(15),
    "period to print stats in minutes");

  o("log-level", po::value<std::string>()->default_value("info"),
    "debug | trace");

  o("print-rpc-stats", po::value<bool>()->default_value(true),
    "if false, --rpc-stats-period-mins is ignored");

  o("print-rpc-histogram-on-exit", po::value<bool>()->default_value(true),
    "if false, no server_hdr.txt will be printed");
}
