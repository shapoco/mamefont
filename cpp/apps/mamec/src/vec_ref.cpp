#include <iostream>

#include "mamec/vec_ref.hpp"

namespace mamefont::mamec {

void dumpByteArray(const std::vector<uint8_t> &arr, const std::string &indent,
                   int offset, int length) {
  const VecRef vecRef(arr, mf::PixelFormat::BW_1BIT);
  dumpByteArray(vecRef, indent, offset, length);
}

void dumpByteArray(const VecRef &arr, const std::string &indent, int offset,
                   int length) {
  size_t n = length < 0 ? arr.size : length;
  for (size_t i = 0; i < n; i++) {
    if (i % 32 == 0) std::cout << indent;
    std::cout << u2x8(arr[offset + i]) << " ";
    if ((i + 1) % 32 == 0 || i == n - 1) std::cout << std::endl;
  }
}

}  // namespace mamefont::mamec