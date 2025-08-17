#pragma once

#include "mamefont/bit_field.hpp"
#include "mamefont/mamefont_common.hpp"

#ifdef MAMEFONT_DEBUG

namespace mamefont {

enum class Operator : int8_t {
  NONE = 0,
  RPT,
  CPY,
  XOR,
  SFT,
  SFI,
  LUP,
  LUD,
  LDI,
  CPX,
  ABO,
  COUNT,
};

static MAMEFONT_INLINE constexpr uint8_t baseCodeOf(Operator op) {
  switch (op) {
    case Operator::RPT: return 0xE0;
    case Operator::CPY: return 0x40;
    case Operator::XOR: return 0xF0;
    case Operator::SFT: return 0x00;
    case Operator::SFI: return 0x68;
    case Operator::LUP: return 0x80;
    case Operator::LUD: return 0xC0;
    case Operator::LDI: return 0x60;
    case Operator::CPX: return 0x40;
    case Operator::ABO: return 0xFF;
    default:
#ifdef MAMEFONT_EXCEPTIONS
      throw std::invalid_argument("Unknown opcode");
#else
      return 0xFF;
#endif
  }
}

static MAMEFONT_INLINE constexpr uint8_t instSizeOf(Operator op) {
  switch (op) {
    case Operator::RPT:
    case Operator::CPY:
    case Operator::XOR:
    case Operator::SFT:
    case Operator::LUP:
    case Operator::LUD:
    case Operator::ABO: return 1;
    case Operator::SFI:
    case Operator::LDI: return 2;
    case Operator::CPX: return 3;
    default:
#ifdef MAMEFONT_EXCEPTIONS
      throw std::invalid_argument("Unknown opcode");
#else
      return 0xFF;
#endif
  }
}

struct LUP {
  static constexpr uint8_t SIZE = 1;
  using Index = BitField<uint8_t, uint8_t, 0, 0, 6>;
};

struct LUD {
  static constexpr uint8_t SIZE = 1;
  using Index = BitField<uint8_t, uint8_t, 0, 0, 4>;
  using Step = BitFlag<0, 4>;
};

struct LDI {
  static constexpr uint8_t SIZE = 2;
  using Fragment = BitField<uint8_t, uint8_t, 1, 0, 8>;
};

struct SFT {
  static constexpr uint8_t SIZE = 1;
  using RepeatCount = BitField<uint8_t, uint8_t, 0, 0, 2, 1>;
  using Size = BitField<uint8_t, uint8_t, 0, 2, 2, 1>;
  using PostSet = BitFlag<0, 4>;
  using Right = BitFlag<0, 5>;
};

struct SFI {
  static constexpr uint8_t SIZE = 2;
  using RepeatCount = BitField<uint8_t, uint8_t, 1, 0, 3, 1>;
  using PreShift = BitFlag<1, 3>;
  using PostSet = BitFlag<1, SFT::PostSet::POS>;
  using Right = BitFlag<1, SFT::Right::POS>;
  using Period = BitField<uint8_t, uint8_t, 1, 6, 2, 2>;
};

struct RPT {
  static constexpr uint8_t SIZE = 1;
  using RepeatCount = BitField<uint8_t, uint8_t, 0, 0, 4, 1>;
};

struct XOR {
  static constexpr uint8_t SIZE = 1;
  using Pos = BitField<uint8_t, uint8_t, 0, 0, 3>;
  using Width2Bit = BitFlag<0, 3>;
};

struct CPY {
  static constexpr uint8_t SIZE = 1;
  using Length = BitField<uint8_t, uint8_t, 0, 0, 3, 1>;
  using Offset = BitField<uint8_t, uint8_t, 0, 3, 2>;
  using ByteReverse = BitFlag<0, 5>;
};

struct CPX {
  static constexpr uint8_t SIZE = 3;
  using Offset = BitField<uint16_t, uint16_t, 0, 0, 9>;
  using Inverse = BitFlag<1, 1>;
  using Length = BitField<uint8_t, uint8_t, 1, 2, 4, 4, 4>;
  using ByteReverse = BitFlag<1, 6>;
  using PixelReverse = BitFlag<1, 7>;
};

struct Instruction {
  uint8_t length = 0;
  uint8_t code[3] = {0x00, 0x00, 0x00};

  Instruction() = default;
  Instruction(uint8_t b1) : length(1), code{b1, 0x00, 0x00} {}
  Instruction(uint8_t b1, uint8_t b2) : length(2), code{b1, b2, 0x00} {}
  Instruction(uint8_t b1, uint8_t b2, uint8_t b3)
      : length(3), code{b1, b2, b3} {}
};

#ifdef MAMEFONT_DEBUG
const char *mnemonicOf(Operator op);
#endif

#ifdef MAMEFONT_INCLUDE_IMPL

#ifdef MAMEFONT_DEBUG
const char *mnemonicOf(Operator op) {
  switch (op) {
    case Operator::NONE: return "(None)";
    case Operator::RPT: return "RPT";
    case Operator::CPY: return "CPY";
    case Operator::XOR: return "XOR";
    case Operator::SFT: return "SFT";
    case Operator::SFI: return "SFI";
    case Operator::LUP: return "LUP";
    case Operator::LUD: return "LUD";
    case Operator::LDI: return "LDI";
    case Operator::CPX: return "CPX";
    case Operator::ABO: return "ABO";
    default: return "(Unknown)";
  }
}
#endif

#endif

}  // namespace mamefont

#endif
