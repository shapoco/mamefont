#pragma once

#include <stdint.h>

#ifdef MAMEFONT_DEBUG
#include <stdio.h>
#include <string.h>
#endif

#define MAMEFONT_ALWAYS_INLINE inline __attribute__((always_inline))
#define MAMEFONT_NOINLINE inline __attribute__((noinline))

namespace mamefont {

static constexpr uint16_t ENTRYPOINT_DUMMY = 0xffff;
static constexpr uint8_t FRAGMENT_SIZE = 8;

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

static constexpr uint8_t CPX_BIT_REVERSAL_MASK = (1 << 7);
static constexpr uint8_t CPX_BYTE_REVERSAL_MASK = (1 << 6);
static constexpr uint8_t CPX_LENGTH_MASK = ((1 << 4) - 1) << 2;
static constexpr uint8_t CPX_LENGTH_BIAS = 4;
static constexpr uint8_t CPX_INVERSE_MASK = (1 << 1);
static constexpr uint8_t CPX_OFFSET_H_MASK = (1 << 0);

enum FontFlags : uint8_t {
  FONT_FLAG_VERTICAL_FRAGMENT = 0x80,
  FONT_FLAG_MSB1ST = 0x40,
  FONT_FLAG_SHRINKED_GLYPH_TABLE = 0x20,
};

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
};

struct GlyphDimensions {
  uint8_t width;
  int8_t xSpacing;
  uint8_t xNegativeOffset;
};

template <typename T, int POS, int WIDTH, T BIAS = 0, T STEP = 1>
struct Field {
  static MAMEFONT_ALWAYS_INLINE T read(uint8_t value) {
    if (POS != 0) {
      value >>= POS;
    }
    value &= ((1 << WIDTH) - 1);
    if (STEP == 1) {
      return BIAS + value;
    } else {
      return BIAS + STEP * value;
    }
  }
};

template <int POS>
struct FlagBit {
  static constexpr int MASK = (1 << POS);
  static MAMEFONT_ALWAYS_INLINE bool read(uint8_t value) {
    return 0 != (value & MASK);
  }
};

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

using XOR_WIDTH_2BIT = FlagBit<3>;
using XOR_POS = Field<uint8_t, 0, 3>;

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
const char *getMnemonic(Operator op);
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
