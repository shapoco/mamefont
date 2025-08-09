#pragma once

#include <stdint.h>

#ifdef MAMEFONT_DEBUG
#include <stdio.h>
#include <string.h>
#endif

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#define MAMEFONT_ALWAYS_INLINE inline __attribute__((always_inline))
#define MAMEFONT_NOINLINE __attribute__((noinline))

namespace mamefont {

static constexpr uint16_t ENTRYPOINT_DUMMY = 0xffff;
static constexpr uint8_t FRAGMENT_SIZE = 8;
static constexpr uint8_t MAX_LUT_SIZE = 64;

static constexpr uint8_t OFST_FORMAT_VERSION = 0;
static constexpr uint8_t OFST_FONT_FLAGS = 1;
static constexpr uint8_t OFST_FIRST_CODE = 2;
static constexpr uint8_t OFST_GLYPH_TABLE_LEN = 3;
static constexpr uint8_t OFST_LUT_SIZE = 4;
static constexpr uint8_t OFST_FONT_DIMENSION_0 = 5;
static constexpr uint8_t OFST_FONT_DIMENSION_1 = 6;
static constexpr uint8_t OFST_FONT_DIMENSION_2 = 7;

static constexpr uint8_t OFST_GLYPH_TABLE = 8;

static constexpr uint8_t OFST_ENTRY_POINT = 0;
static constexpr uint8_t OFST_GLYPH_DIMENSION_0 = 2;
static constexpr uint8_t OFST_GLYPH_DIMENSION_1 = 3;

enum class Status : uint8_t {
  SUCCESS = 0,
  CHAR_CODE_OUT_OF_RANGE,
  GLYPH_NOT_DEFINED,
  UNKNOWN_OPCODE,
  ABORTED_BY_ABO,
};

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
};

static MAMEFONT_ALWAYS_INLINE constexpr uint8_t baseCodeOf(Operator op) {
  switch (op) {
    case Operator::RPT:
      return 0xE0;
    case Operator::CPY:
      return 0x40;
    case Operator::XOR:
      return 0xF0;
    case Operator::SFT:
      return 0x00;
    case Operator::SFI:
      return 0x68;
    case Operator::LUP:
      return 0x80;
    case Operator::LUD:
      return 0xC0;
    case Operator::LDI:
      return 0x60;
    case Operator::CPX:
      return 0x40;
    default:
      return 0xFF;  // including ABO
  }
}

struct GlyphDimensions {
  uint8_t width;
  int8_t xSpacing;
  uint8_t xNegativeOffset;
};

template <typename T, int Param_POS, int Param_WIDTH, T Param_MIN = 0,
          T Param_STEP = 1>
struct Field {
  static constexpr int POS = Param_POS;
  static constexpr int WIDTH = Param_WIDTH;
  static constexpr T STEP = Param_STEP;
  static constexpr T MIN = Param_MIN;
  static constexpr T MAX = MIN + ((1 << WIDTH) - 1) * STEP;
  static MAMEFONT_ALWAYS_INLINE T read(uint8_t value) {
    if (POS != 0) {
      value >>= POS;
    }
    value &= ((1 << WIDTH) - 1);
    if (STEP == 1) {
      return MIN + value;
    } else {
      return MIN + STEP * value;
    }
  }
  static MAMEFONT_ALWAYS_INLINE uint8_t place(T value) {
#ifdef MAMEFONT_EXCEPTIONS
    if (value < MIN || value > MAX) {
      throw std::out_of_range("Value out of range");
    }
    if ((value - MIN) % STEP != 0) {
      throw std::invalid_argument("Value is not a multiple of step");
    }
#endif
    return ((value - MIN) / STEP) << POS;
  }
};

template <int POS>
struct FlagBit {
  static constexpr int MASK = (1 << POS);
  static MAMEFONT_ALWAYS_INLINE bool read(uint8_t value) {
    return 0 != (value & MASK);
  }
  static MAMEFONT_ALWAYS_INLINE uint8_t place(bool value) {
    return value ? MASK : 0;
  }
};

using FONT_DIM_FONT_HEIGHT = Field<uint8_t, 0, 6, 1>;
using FONT_DIM_Y_SPACING = Field<uint8_t, 0, 6>;
using FONT_DIM_MAX_GLYPH_WIDTH = Field<uint8_t, 0, 6, 1>;

using FONT_FLAG_VERTICAL_FRAGMENT = FlagBit<7>;
using FONT_FLAG_MSB1ST = FlagBit<6>;
using FONT_FLAG_SHRINKED_GLYPH_TABLE = FlagBit<5>;

using GLYPH_DIM_WIDTH = Field<uint8_t, 0, 6, 1>;
using GLYPH_DIM_X_SPACING = Field<int8_t, 0, 5, -16>;
using GLYPH_DIM_X_NEGATIVE_OFFSET = Field<uint8_t, 5, 3>;

using GLYPH_SHRINKED_DIM_WIDTH = Field<uint8_t, 0, 4, 1>;
using GLYPH_SHRINKED_DIM_X_SPACING = Field<int8_t, 4, 2>;
using GLYPH_SHRINKED_DIM_X_NEGATIVE_OFFSET = Field<uint8_t, 6, 2>;

using LUP_INDEX = Field<uint8_t, 0, 6>;

using LUD_INDEX = Field<uint8_t, 0, 4>;
using LUD_STEP = FlagBit<4>;

using SFT_REPEAT_COUNT = Field<uint8_t, 0, 2, 1>;
using SFT_SIZE = Field<uint8_t, 2, 2, 1>;
using SFT_POST_SET = FlagBit<4>;
using SFT_RIGHT = FlagBit<5>;

using SFI_REPEAT_COUNT = Field<uint8_t, 0, 3, 1>;
using SFI_PRE_SHIFT = FlagBit<3>;
using SFI_POST_SET = SFT_POST_SET;
using SFI_RIGHT = SFT_RIGHT;
using SFI_PERIOD = Field<uint8_t, 6, 2, 2>;

using RPT_REPEAT_COUNT = Field<uint8_t, 0, 4, 1>;

using XOR_POS = Field<uint8_t, 0, 3>;
using XOR_WIDTH_2BIT = FlagBit<3>;

using CPY_LENGTH = Field<uint8_t, 0, 3, 1>;
using CPY_OFFSET = Field<uint8_t, 3, 2>;
using CPY_BYTE_REVERSE = FlagBit<5>;

using CPX_OFFSET_H = Field<uint8_t, 0, 1>;
using CPX_INVERSE = FlagBit<1>;
static constexpr uint8_t CPX_LENGTH_MASK = ((1 << 4) - 1) << 2;
static constexpr uint8_t CPX_LENGTH_BIAS = 4;
using CPX_BYTE_REVERSE = FlagBit<6>;
using CPX_BIT_REVERSE = FlagBit<7>;

using fragment_t = uint8_t;

#ifdef MAMEFONT_FRAGMENT_INDEX_8BIT
using frag_index_t = int8_t;
#else
using frag_index_t = int16_t;
#endif

#ifdef MAMEFONT_PROGRAM_COUNTER_8BIT
using prog_cntr_t = uint8_t;
#else
using prog_cntr_t = uint16_t;
#endif

#ifdef MAMEFONT_DEBUG
const char *mnemonicOf(Operator op);
#endif

static MAMEFONT_ALWAYS_INLINE uint8_t getRightMask(uint8_t width) {
  switch (width) {
    case 0:
      return 0x00;
    case 1:
      return 0x01;
    case 2:
      return 0x03;
    case 3:
      return 0x07;
    case 4:
      return 0x0f;
    case 5:
      return 0x1f;
    case 6:
      return 0x3f;
    case 7:
      return 0x7f;
    default:
      return 0xff;
  }
}

}  // namespace mamefont
