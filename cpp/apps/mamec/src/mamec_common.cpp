#include <string>

#include "mamec/mamec_common.hpp"
#include "mamec/vec_ref.hpp"

namespace mamefont::mamec {

size_t objectId = 0;

std::string formatChar(int code) {
  char buf[32];
  if (0x20 <= code && code <= 0x7E) {
    snprintf(buf, sizeof(buf), "'%c' (0x%02X)", (char)code, code);
  } else {
    snprintf(buf, sizeof(buf), "'\\x%02X'", code);
  }
  return buf;
}

std::string byteToHexStr(uint8_t byte) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%02X", byte);
  return buf;
}

bool maskedEqual(const VecRef &a, const VecRef &b, const VecRef &mask) {
  if (a.size != mask.size || b.size != mask.size) {
    throw std::runtime_error(
        "size mismatch in maskedEqual(): a.size()=" + std::to_string(a.size) +
        ", b.size()=" + std::to_string(b.size) +
        ", mask.size()=" + std::to_string(mask.size));
  }
  for (size_t i = 0; i < a.size; ++i) {
    if ((a[i] ^ b[i]) & mask[i]) return false;
  }
  return true;
}

}  // namespace mamefont::mamec
