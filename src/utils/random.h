#pragma once
#include <random>

#include <core/sstring.hh>

namespace smf {
class random {
 public:
  random() {
    std::random_device rd;
    rand_.seed(rd());
  }

  uint64_t next() { return dist_(rand_); }
  sstring next_sstring(uint32_t size) {
    static const sstring dict = "abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "1234567890"
                                "!@#$%^&*()"
                                "`~-_=+[{]}\\|;:'\",<.>/? ";
    const static std::uniform_int_distribution<> dict_dist(0, dict.size());

    sstring retval;
    retval.resize(size);
    // this could use a simd
    for (uint32_t i = 0; i < size; ++i) {
      retval[i] = dict[dict_dist(rand_)];
    }
    return retval;
  }

 private:
  // mersenne twister
  // TODO(agallego) - replace w/ lemire's stuff
  std::mt19937 rand_;
  // by default range [0, MAX]
  std::uniform_int_distribution<uint64_t> dist_;
};

}  // namespace smf
