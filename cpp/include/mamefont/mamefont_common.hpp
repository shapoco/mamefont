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
struct BitField {
  static constexpr int POS = Param_POS;
  static constexpr int WIDTH = Param_WIDTH;
  static constexpr T STEP = Param_STEP;
  static constexpr T MIN = Param_MIN;
  static constexpr T MAX = MIN + ((1 << WIDTH) - 1) * STEP;
  static MAMEFONT_ALWAYS_INLINE T read(uint8_t value) {
    if ((1 << POS) == STEP) {
      value &= ((1 << WIDTH) - 1) << POS;
      return MIN + value;
    } else {
      if (POS != 0) {
        value >>= POS;
      }
      value &= ((1 << WIDTH) - 1);
      return MIN + STEP * value;
    }
  }
  static MAMEFONT_ALWAYS_INLINE uint8_t place(int value) {
#ifdef MAMEFONT_EXCEPTIONS
    if (value < MIN || MAX < value) {
      throw std::out_of_range("Value " + std::to_string(value) +
                              " out of range " + std::to_string(MIN) + ".." +
                              std::to_string(MAX));
    }
    if ((value - MIN) % STEP != 0) {
      throw std::invalid_argument("Value " + std::to_string(value) +
                                  " is not a multiple of step " +
                                  std::to_string(STEP));
    }
#endif
    return ((value - MIN) / STEP) << POS;
  }
};

template <int Param_POS>
struct BitFlag {
  static constexpr int POS = Param_POS;
  static constexpr int MASK = (1 << POS);
  static MAMEFONT_ALWAYS_INLINE bool read(uint8_t value) {
    return 0 != (value & MASK);
  }
  static MAMEFONT_ALWAYS_INLINE uint8_t place(bool value) {
    return value ? MASK : 0;
  }
};

using FONT_DIM_FONT_HEIGHT = BitField<uint8_t, 0, 6, 1>;
using FONT_DIM_Y_SPACING = BitField<uint8_t, 0, 6>;
using FONT_DIM_MAX_GLYPH_WIDTH = BitField<uint8_t, 0, 6, 1>;

using FONT_FLAG_VERTICAL_FRAGMENT = BitFlag<7>;
using FONT_FLAG_MSB1ST = BitFlag<6>;
using FONT_FLAG_SHRINKED_GLYPH_TABLE = BitFlag<5>;

using GLYPH_DIM_WIDTH = BitField<uint8_t, 0, 6, 1>;
using GLYPH_DIM_X_SPACING = BitField<int8_t, 0, 5, -16>;
using GLYPH_DIM_X_NEGATIVE_OFFSET = BitField<uint8_t, 5, 3>;

using GLYPH_SHRINKED_DIM_WIDTH = BitField<uint8_t, 0, 4, 1>;
using GLYPH_SHRINKED_DIM_X_SPACING = BitField<int8_t, 4, 2>;
using GLYPH_SHRINKED_DIM_X_NEGATIVE_OFFSET = BitField<uint8_t, 6, 2>;

using LUP_INDEX = BitField<uint8_t, 0, 6>;

using LUD_INDEX = BitField<uint8_t, 0, 4>;
using LUD_STEP = BitFlag<4>;

using SFT_REPEAT_COUNT = BitField<uint8_t, 0, 2, 1>;
using SFT_SIZE = BitField<uint8_t, 2, 2, 1>;
using SFT_POST_SET = BitFlag<4>;
using SFT_RIGHT = BitFlag<5>;

using SFI_REPEAT_COUNT = BitField<uint8_t, 0, 3, 1>;
using SFI_PRE_SHIFT = BitFlag<3>;
using SFI_POST_SET = SFT_POST_SET;
using SFI_RIGHT = SFT_RIGHT;
using SFI_PERIOD = BitField<uint8_t, 6, 2, 2>;

using RPT_REPEAT_COUNT = BitField<uint8_t, 0, 4, 1>;

using XOR_POS = BitField<uint8_t, 0, 3>;
using XOR_WIDTH_2BIT = BitFlag<3>;

using CPY_LENGTH = BitField<uint8_t, 0, 3, 1>;
using CPY_OFFSET = BitField<uint8_t, 3, 2>;
using CPY_BYTE_REVERSE = BitFlag<5>;

static constexpr uint8_t CPX_OFFSET_WIDTH = 9;
static constexpr uint8_t CPX_OFFSET_STEP = 1;
static constexpr uint16_t CPX_OFFSET_MIN = 0;
static constexpr uint16_t CPX_OFFSET_MAX =
    CPX_OFFSET_MIN + ((1 << CPX_OFFSET_WIDTH) - 1) * CPX_OFFSET_STEP;
using CPX_OFFSET_H = BitField<uint8_t, 0, CPX_OFFSET_WIDTH - 8>;
using CPX_INVERSE = BitFlag<1>;
using CPX_LENGTH = BitField<uint8_t, 2, 4, 4, 4>;
using CPX_BYTE_REVERSE = BitFlag<6>;
using CPX_BIT_REVERSE = BitFlag<7>;

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
