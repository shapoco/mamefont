#include <iomanip>
#include <iostream>
#include <string>

#include "mamec/mamec_common.hpp"
#include "mamec/vec_ref.hpp"

namespace mamefont::mamec {

size_t objectId = 0;

std::string i2s(int value, int width) {
  std::ostringstream oss;
  oss << std::setw(width) << value;
  return oss.str();
}

std::string f2s(float value, int width, int precision) {
  std::ostringstream oss;
  oss << std::setw(width) << std::fixed << std::setprecision(precision)
      << value;
  return oss.str();
}

std::string c2s(int code) {
  char buf[32];
  if (0x20 <= code && code <= 0x7E) {
    snprintf(buf, sizeof(buf), "'%c' (0x%02X)", (char)code, code);
  } else {
    snprintf(buf, sizeof(buf), "'\\x%02X'", code);
  }
  return buf;
}

std::string s2s(std::string s, int width) {
  std::ostringstream oss;
  oss << std::setw(width) << std::left << s;
  return oss.str();
}

std::string u2x8(uint8_t value) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%02X", value);
  return buf;
}

std::string u2x16(uint16_t value) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%04X", value);
  return buf;
}

void dumpCStyleArrayContent(std::ostream &os, const std::vector<uint8_t> &arr,
                            const std::string &indent, int offset, int length,
                            bool hex, bool endsWithComma) {
  size_t n = length < 0 ? arr.size() : length;
  for (size_t i = 0; i < n; i++) {
    if (i % 16 == 0) os << indent;
    if (hex)
      os << "0x" << u2x8(arr[offset + i]);
    else
      os << std::dec << (int)arr[offset + i];
    if (i < n - 1 || endsWithComma) os << ",";
    if ((i + 1) % 16 != 0 && i < n - 1) {
      os << " ";
    } else {
      os << std::endl;
    }
  }
}

}  // namespace mamefont::mamec
