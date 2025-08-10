#pragma once

#include <stdint.h>
#include <vector>

#include <mamefont/mamefont.hpp>

namespace mf = mamefont;
using frag_t = mf::frag_t;

namespace mamefont::mamec {

extern size_t objectId;

static inline size_t nextObjectId() { return objectId++; }

static inline bool maskedEqual(frag_t a, frag_t b, frag_t mask,
                               uint8_t cpxFlags = 0) {
  if (mf::CPX_BIT_REVERSE::read(cpxFlags)) {
    a = mf::reverseBits(a);
  }
  if (mf::CPX_INVERSE::read(cpxFlags)) {
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

std::string formatChar(int code);
std::string byteToHexStr(uint8_t byte);
void dumpByteArray(const std::vector<uint8_t> &arr, const std::string &indent);

}  // namespace mamefont::mamec
