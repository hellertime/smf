#pragma once

#include <chrono>

#include "histogram/histogram.h"

namespace smf {
namespace load_gen {

struct load_gen_stats {
  load_gen_duration(uint64_t reqs, std::chrono::duration _d, smf::histogram &&h)
    : num_of_req(reqs), test_duration(_d), test_histogram(std::move(h)) {}

  uint64_t              num_of_req;
  std::chrono::duration test_duration;
  smf::histogram        test_histogram;

  inline uint64_t duration_in_millis() {
    namespace co = std::chrono;
    return co::duration_cast<co::milliseconds>(duration.count());
  }

  inline uint64_t qps() {
    auto       milli = duration_in_millis();
    const auto reqs  = static_cast<double>(num_of_req);

    // some times the test run under 1 millisecond
    const auto safe_denom = std::max<uint64_t>(milli, 1);
    const auto denom      = static_cast<double>(safe_denom);

    auto queries_per_milli = reqs / denom;

    return queries_per_milli * 1000.0;
  }
};

}  // namespace load_gen
}  // namespace smf
