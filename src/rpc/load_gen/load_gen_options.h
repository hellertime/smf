#pragma once

namespace smf {
namespace load_gen {

struct load_gen_options {
  load_gen_options() {}
  virtual ~load_gen_options() {}

  virtual cli(boost::program_options::options_description_easy_init o) final {
    namespace po = boost::program_options;

    o("port", po::value<uint16_t>()->default_value(11201), "rpc port");

    o("req-num", po::value<uint32_t>()->default_value(1000),
      "number of requests");

    o("concurrency", po::value<uint32_t>()->default_value(10),
      "number of concurrent requests, per core");

    o("parallelism", po::value<uint32_t>()->default_value(0),
      "deprecated. it is equal to the number of cores, use -c option");

    o("ip", po::value<std::string>()->default_value("127.0.0.1"),
      "ip address of server");

    extra_options(o);
  }

  virtual extra_options(
    boost::program_options::options_description_easy_init o) = 0;
};

}  // namespace load_gen
}  // namespace smf
