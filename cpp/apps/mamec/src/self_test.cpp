#include <string>

#include "mamec/encoder.hpp"
#include "mamec/mamec_common.hpp"
#include "mamec/self_test.hpp"

namespace mamefont::mamec {

void runSelfTests() {
  for (bool right : {false, true}) {
    for (bool postSet : {false, true}) {
      for (int frag = 0; frag <= 255; frag++) {
        uint16_t enc = Encoder::encodeShiftState2bpp(frag, right, postSet);
        uint16_t dec = mf::encodeShiftState2bpp(frag, right, postSet);
        if (enc != dec) {
          throw std::runtime_error(
              "encodeShiftState2bpp mismatch(): right=" +
              std::to_string(right ? 1 : 0) +
              ", postSet=" + std::to_string(postSet ? 1 : 0) + ", frag=0x" +
              u2x8(frag) + ", encoder output=0x" + u2x16(enc) +
              ", decoder output=0x" + u2x16(dec));
        }
      }
    }
  }

  for (int frag = 0; frag <= 255; frag++) {
    uint16_t enc = Encoder::decodeShiftState2bpp(frag);
    uint16_t dec = mf::decodeShiftState2bpp(frag);
    if (enc != dec) {
      throw std::runtime_error("decodeShiftState2bpp mismatch(): frag=0x" +
                               u2x8(frag) + ", encoder output=0x" + u2x16(enc) +
                               ", decoder output=0x" + u2x16(dec));
    }
  }
}

}  // namespace mamefont::mamec
