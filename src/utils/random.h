#pragma once
#include <random>
#include <string>

#include <core/sstring.hh>

namespace smf {
class random {
 public:
  random() {
    std::random_device rd;
    rand_.seed(rd());
  }

  uint64_t next() { return dist_(rand_); }

  template <typename char_type = char>
  std::basic_string<char_type> next_str(uint32_t size) {
    static const std::string kDict = "abcdefghijklmnopqrstuvwxyz"
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "1234567890"
                                     "!@#$%^&*()"
                                     "`~-_=+[{]}\\|;:'\",<.>/? ";
    static const std::uniform_int_distribution<> kDictDist(0, dict.size());

    std::basic_string<char_type> retval;
    retval.resize(size);
    // this could use a simd
    for (uint32_t i = 0; i < size; ++i) {
      retval[i] = kDict[kDictDist(rand_)];
    }
    return retval;
  }

 private:
  std::mt19937 rand_;
  // by default range [0, MAX]
  std::uniform_int_distribution<uint64_t> dist_;
};

}  // namespace smf
