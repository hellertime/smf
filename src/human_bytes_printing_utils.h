#pragma once
#include <ostream>
namespace smf {

template <typename T> static void human_bytes(std::ostream &o, T t) {
  auto const orig_precision = o.precision();
  o << std::fixed << std::setprecision(3);
  if(t < 100) {
    if(t < 1) {
      t = 0;
    }
    o << t << " bytes";
  } else if((t /= T(1024)) < T(1000)) {
    o << t << " KB";
  } else if((t /= T(1024)) < T(1000)) {
    o << t << " MB";
  } else if((t /= T(1024)) < T(1000)) {
    o << t << " GB";
  } else {
    o << t << " TB";
  }
  std::setprecision(orig_precision);
}


} // namespace smf