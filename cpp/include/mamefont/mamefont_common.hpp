#pragma once

#include <stdint.h>

#ifdef MAMEFONT_STM_DEBUG
#include <stdio.h>
#include <string.h>
#endif

#define MAMEFONT_ALWAYS_INLINE inline __attribute__((always_inline))

namespace mamefont {

static constexpr int16_t ENTRYPOINT_DUMMY = 0xffff;
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

static constexpr uint8_t CPX_MASK_LUT_INDEX = (1 << 6) - 1;
static constexpr uint8_t CPX_MASK_RECENT_OFFSET = (1 << 6) - 1;
static constexpr uint8_t CPX_BIT_POS_DISTANT_LANE_OFFSET = 4;
static constexpr uint8_t CPX_MASK_DISTANT_LANE_OFFSET =
    ((1 << 2) - 1) << CPX_BIT_POS_DISTANT_LANE_OFFSET;
static constexpr uint8_t CPX_MASK_DISTANT_FRAGMENT_OFFSET = (1 << 4) - 1;

enum class CpxSource : uint8_t {
  LUT,
  RECENT,
  DISTANT,
};

static constexpr uint8_t CPX_MASK_BIT_REVERSAL = (1 << 6);
static constexpr uint8_t CPX_MASK_BYTE_REVERSAL = (1 << 7);
static constexpr uint8_t CPX_MASK_LENGTH = (1 << 6) - 1;

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
};

enum class Operator : int8_t {
  NONE = 0,
  RPT,
  CPY,
  REV,
  XOR,
  SLC,
  SLS,
  SRC,
  SRS,
  LUP,
  LUD,
  LDI,
  CPX,
};

using fragment_t = uint8_t;

#ifdef MAMEFONT_32BIT_ADDR
using frag_index_t = int32_t;
#elifdef MAMEFONT_8BIT_ADDR
using frag_index_t = int8_t;
#else
using frag_index_t = int16_t;
#endif

#ifdef MAMEFONT_8BIT_PC
using prog_cntr_t = uint8_t;
#else
using prog_cntr_t = uint16_t;
#endif

}  // namespace mamefont
