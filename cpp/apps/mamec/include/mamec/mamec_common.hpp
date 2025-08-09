#pragma once

#include <stdint.h>

#define MAMEFONT_EXCEPTIONS
#include <mamefont/mamefont.hpp>

#include "mamec/vec_ref.hpp"

namespace mf = mamefont;
using fragment_t = mf::fragment_t;

namespace mamefont::mamec {

extern  size_t objectId;

static inline size_t nextObjectId() {
  return objectId++;
}

std::string formatChar(int code);
std::string byteToHexStr(uint8_t byte);
bool maskedEqual(const VecRef &a, const VecRef &b, const VecRef &mask);

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




}  // namespace mamefont::mamec
