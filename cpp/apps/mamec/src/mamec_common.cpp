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

}  // namespace mamefont::mamec
