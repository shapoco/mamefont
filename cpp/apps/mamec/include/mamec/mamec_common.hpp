#pragma once

#include <stdint.h>
#include <sstream>
#include <vector>

#include <mamefont/mamefont.hpp>

namespace mf = mamefont;

namespace mamefont::mamec {

using frag_t = mf::frag_t;

enum class FileType {
  NONE,
  BMP,
  PNG,
  JPEG,
  MAME_JSON,
  MAME_HPP,
};

struct Duplication {
  int sourceCode;
  int offset;
  int size;

  Duplication()
      : sourceCode(-1), offset(0), size(0) {}
};

extern size_t objectId;

static inline size_t nextObjectId() { return objectId++; }

static inline bool maskedEqual(frag_t a, frag_t b, frag_t mask,
                               uint8_t cpxFlags = 0) {
  if (mf::CPX::BitReverse::read(cpxFlags)) {
    a = mf::reverseBits(a);
  }
  if (mf::CPX::Inverse::read(cpxFlags)) {
    a = ~a;
  }
  return ((a ^ b) & mask) == 0;
}

static inline constexpr int baseCostOf(mf::Operator op) {
  switch (op) {
    case mf::Operator::RPT:
      return 1001;
    case mf::Operator::CPY:
      return 1003;
    case mf::Operator::XOR:
      return 1004;
    case mf::Operator::SFT:
      return 1005;
    case mf::Operator::LUP:
      return 1006;
    case mf::Operator::LUD:
      return 1006;
    case mf::Operator::SFI:
      return 2000;
    case mf::Operator::LDI:
      return 2001;
    case mf::Operator::CPX:
      return 3100;
    default:
      return 9999;
  }
}

std::string i2s(int value, int width = 4);
std::string f2s(float value, int width = 5, int precision = 2);
std::string c2s(int code);
std::string s2s(std::string s, int width = 4);
static inline std::string yn(bool value) { return value ? "Yes" : "No"; }
std::string byteToHexStr(uint8_t byte);
void dumpByteArray(const std::vector<uint8_t> &arr, const std::string &indent,
                   int offset = 0, int length = -1);
void dumpCStyleArrayContent(std::ostream &os, const std::vector<uint8_t> &arr,
                            const std::string &indent, int offset = 0,
                            int length = -1, bool hex = true,
                            bool endsWithComma = true);

}  // namespace mamefont::mamec
