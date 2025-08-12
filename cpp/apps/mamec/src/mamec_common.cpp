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
  oss << std::setw(width) << std::fixed << std::setprecision(precision) << value;
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

std::string byteToHexStr(uint8_t byte) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%02X", byte);
  return buf;
}

void dumpByteArray(const std::vector<uint8_t> &arr, const std::string &indent,
                   int offset, int length) {
  size_t n = length < 0 ? arr.size() : length;
  for (size_t i = 0; i < n; i++) {
    if (i % 32 == 0) std::cout << indent;
    std::cout << byteToHexStr(arr[offset + i]) << " ";
    if ((i + 1) % 32 == 0 || i == n - 1) std::cout << std::endl;
  }
}

void dumpCStyleArrayContent(std::ostream &os, const std::vector<uint8_t> &arr,
                            const std::string &indent, int offset, int length,
                            bool hex, bool endsWithComma) {
  size_t n = length < 0 ? arr.size() : length;
  for (size_t i = 0; i < n; i++) {
    if (i % 16 == 0) os << indent;
    if (hex)
      os << "0x" << byteToHexStr(arr[offset + i]);
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
