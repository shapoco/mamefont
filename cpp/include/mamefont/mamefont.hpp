#pragma once

#include "mamefont/mamefont_common.hpp"

#ifdef MAMEFONT_USE_PROGMEM
#include <avr/pgmspace.h>
#define mamefont_readBlobU8 pgm_read_byte
#else
#define mamefont_readBlobU8(addr) (*(const uint8_t *)(addr))
#endif

namespace mamefont {

static uint8_t reverseBits(uint8_t b) {
  b = ((b & 0x55) << 1) | ((b & 0xaa) >> 1);
  b = ((b & 0x33) << 2) | ((b & 0xcc) >> 2);
  b = (b << 4) | (b >> 4);
  return b;
}

#ifdef MAMEFONT_USE_PROGMEM
#define mamefont_readBlobU8 pgm_read_byte
#else
#define mamefont_readBlobU8(addr) (*(const uint8_t *)(addr))
#endif

static MAMEFONT_ALWAYS_INLINE uint16_t readBlobU16(const uint8_t *ptr) {
  uint8_t lo = mamefont_readBlobU8(ptr);
  uint8_t hi = mamefont_readBlobU8(ptr + 1);
  return (static_cast<uint16_t>(hi) << 8) | lo;
}

struct Glyph {
  const uint8_t *blob;
  bool isShrinked;

  Glyph() : blob(nullptr), isShrinked(false) {}
  Glyph(const uint8_t *blob, bool isShrinked)
      : blob(blob), isShrinked(isShrinked) {}

  MAMEFONT_ALWAYS_INLINE uint16_t entryPoint() const {
    if (isShrinked) {
      return ((uint16_t)mamefont_readBlobU8(blob)) << 1;
    } else {
      return readBlobU16(blob);
    }
  }

  MAMEFONT_ALWAYS_INLINE bool isValid() const {
    return (blob != nullptr) && (readBlobU16(blob) != ENTRYPOINT_DUMMY);
  }

  MAMEFONT_ALWAYS_INLINE void getDimensions(GlyphDimensions *dims) const {
    if (isShrinked) {
      uint8_t dim = mamefont_readBlobU8(blob + 1);
      dims->width = GLYPH_SHRINKED_DIM_WIDTH::read(dim);
      dims->xSpacing = GLYPH_SHRINKED_DIM_X_SPACING::read(dim);
      dims->xNegativeOffset = GLYPH_SHRINKED_DIM_X_NEGATIVE_OFFSET::read(dim);
    } else {
      uint8_t dim0 = mamefont_readBlobU8(blob + 2);
      dims->width = GLYPH_DIM_WIDTH::read(dim0);
      uint8_t dim1 = mamefont_readBlobU8(blob + 3);
      dims->xSpacing = GLYPH_DIM_X_SPACING::read(dim1);
      dims->xNegativeOffset = GLYPH_DIM_X_NEGATIVE_OFFSET::read(dim1);
    }
  }

  MAMEFONT_ALWAYS_INLINE uint8_t width() const {
    GlyphDimensions dims;
    getDimensions(&dims);
    return dims.width;
  }
};

class Font {
 public:
  const uint8_t *blob;

  Font(const uint8_t *blob) : blob(blob) {}

  MAMEFONT_ALWAYS_INLINE uint8_t flags() const {
    return mamefont_readBlobU8(blob + OFST_FONT_FLAGS);
  }

  MAMEFONT_ALWAYS_INLINE bool isVerticalFragment() const {
    return FONT_FLAG_VERTICAL_FRAGMENT::read(flags());
  }

  MAMEFONT_ALWAYS_INLINE bool isMsb1st() const {
    return FONT_FLAG_MSB1ST::read(flags());
  }

  MAMEFONT_ALWAYS_INLINE bool isShrinkedGlyphTable() const {
    return FONT_FLAG_SHRINKED_GLYPH_TABLE::read(flags());
  }

  MAMEFONT_ALWAYS_INLINE uint8_t glyphTableEntrySize() const {
    return isShrinkedGlyphTable() ? 2 : 4;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t firstCode() const {
    return mamefont_readBlobU8(blob + OFST_FIRST_CODE);
  }

  MAMEFONT_ALWAYS_INLINE uint8_t numGlyphs() const {
    return mamefont_readBlobU8(blob + OFST_GLYPH_TABLE_LEN) + 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t lastCode() const {
    return firstCode() + numGlyphs() - 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t lutSize() const {
    return mamefont_readBlobU8(blob + OFST_LUT_SIZE) + 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t glyphHeight() const {
    return (mamefont_readBlobU8(blob + OFST_FONT_DIMENSION_0) & 0x3f) + 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t maxGlyphWidth() const {
    return (mamefont_readBlobU8(blob + OFST_FONT_DIMENSION_2) & 0x3f) + 1;
  }

  MAMEFONT_ALWAYS_INLINE bool verticalFragment() const {
#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    return false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    return true;
#else
    return FONT_FLAG_VERTICAL_FRAGMENT::read(flags());
#endif
  }

  MAMEFONT_ALWAYS_INLINE const Glyph glyphTableEntry(uint8_t index) const {
    const uint8_t *ptr = blob + OFST_GLYPH_TABLE;
    ptr += isShrinkedGlyphTable() ? (index << 1) : (index << 2);
    return Glyph(ptr, isShrinkedGlyphTable());
  }

  Status getGlyph(uint8_t c, Glyph * glyph) const {
    uint8_t first = firstCode();
    uint8_t last = lastCode();
    if (c < first || last < c) return Status::CHAR_CODE_OUT_OF_RANGE;
    uint8_t index = c - first;

    auto g = glyphTableEntry(index);
    if (glyph) *glyph = g;

    return g.isValid() ? Status::SUCCESS : Status::GLYPH_NOT_DEFINED;
  }

  MAMEFONT_ALWAYS_INLINE uint16_t lutOffset() const {
    uint16_t n = numGlyphs();
    n <<= 1;
    if (!isShrinkedGlyphTable()) {
      n <<= 1;
    }
    return OFST_GLYPH_TABLE + n;
  }

  MAMEFONT_ALWAYS_INLINE uint16_t microCodeOffset() const {
    return lutOffset() + lutSize();
  }

  frag_index_t getRequiredGlyphBufferSize(const Glyph *glyph,
                                          uint8_t *stride) const {
    uint8_t w = glyph ? glyph->width() : maxGlyphWidth();
    uint8_t h = glyphHeight();
    if (verticalFragment()) {
      h = (h + 7) / 8;
    } else {
      w = (w + 7) / 8;
    }
    *stride = w;
    return w * h;
  }

  frag_index_t getRequiredGlyphBufferSize(uint8_t *stride) const {
    return getRequiredGlyphBufferSize(nullptr, stride);
  }

#ifdef MAMEFONT_DEBUG
  fragment_t getLutEntry(int index) const {
    if (index < 0 || lutSize() <= index) return 0x00;
    return mamefont_readBlobU8(blob + lutOffset() + index);
  }
#endif
};

struct GlyphBuffer {
  uint8_t *data;
  frag_index_t stride;

  GlyphBuffer() : data(nullptr), stride(0) {}
  GlyphBuffer(uint8_t *data, frag_index_t stride)
      : data(data), stride(stride) {}

  MAMEFONT_ALWAYS_INLINE bool getPixel(const Font &font, const Glyph &glyph, int8_t x,
                                       int8_t y) const {
    if (!data || !glyph.isValid()) return false;
    if (x < 0 || glyph.width() <= x) return false;
    if (y < 0 || font.glyphHeight() <= y) return false;

    uint8_t ibit;
    frag_index_t offset;
    if (font.isVerticalFragment()) {
      offset = y / 8 * stride + x;
      ibit = y & 0x07;
    } else {
      offset = x / 8 + y * stride;
      ibit = x & 0x07;
    }
    if (font.isMsb1st()) {
      ibit = 7 - ibit;
    }
    uint8_t mask = 1 << ibit;
    return (data[offset] & mask) != 0;
  }
};

struct AddrRule {
  int8_t lanesPerGlyph;  // Number of lanes per glyph
  int8_t fragsPerLane;   // Number of fragments per lane
#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
  static constexpr uint8_t laneStride = 1;
#else
  frag_index_t laneStride;  // Stride to next lane in bytes
#endif
#ifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
  static constexpr uint8_t fragStride = 1;
#else
  frag_index_t fragStride;  // Stride to next fragment in bytes
#endif
};

struct Cursor {
  frag_index_t offset;      // Offset from start of glyph in bytes
  frag_index_t laneOffset;  // Offset of first fragment of current lane
  int8_t fragIndex;         // Distance from start of current lane in bytes

  void reset() {
    offset = 0;
    laneOffset = 0;
    fragIndex = 0;
  }

  MAMEFONT_ALWAYS_INLINE void add(const AddrRule &rule, frag_index_t delta) {
    if (delta >= 0) {
      while (delta >= rule.fragsPerLane) {
        delta -= rule.fragsPerLane;
        laneOffset += rule.laneStride;
        offset += rule.laneStride;
      }
      fragIndex += delta;
      offset += delta * rule.fragStride;
      if (fragIndex >= rule.fragsPerLane) {
        fragIndex -= rule.fragsPerLane;
        laneOffset += rule.laneStride;
        offset = laneOffset + fragIndex * rule.fragStride;
      }
    } else {
      delta = -delta;
      while (delta >= rule.fragsPerLane) {
        delta -= rule.fragsPerLane;
        laneOffset -= rule.laneStride;
        offset -= rule.laneStride;
      }
      fragIndex -= delta;
      offset -= delta * rule.fragStride;
      if (fragIndex < 0) {
        laneOffset -= rule.laneStride;
        fragIndex += rule.fragsPerLane;
        offset = laneOffset + fragIndex * rule.fragStride;
      }
    }
  }

  MAMEFONT_ALWAYS_INLINE frag_index_t postIncr(const AddrRule &rule) {
    frag_index_t oldOffset = offset;
    offset += rule.fragStride;
    fragIndex++;
    if (fragIndex >= rule.fragsPerLane) {
      fragIndex = 0;
      laneOffset += rule.laneStride;
      offset = laneOffset;
    }
    return oldOffset;
  }

  MAMEFONT_ALWAYS_INLINE frag_index_t preDecr(const AddrRule &rule) {
    offset -= rule.fragStride;
    fragIndex--;
    if (fragIndex < 0) {
      fragIndex = rule.fragsPerLane - 1;
      laneOffset -= rule.laneStride;
      offset = laneOffset + fragIndex * rule.fragStride;
    }
    return offset;
  }
};

class StateMachine {
 private:
  uint8_t flags;
  const fragment_t *lut;
  const uint8_t *bytecode;
  uint8_t glyphHeight;

  uint8_t *buffData;

  int8_t numLanesToGlyphEnd = 0;

  prog_cntr_t programCounter = 0;
  fragment_t lastFragment = 0;

  AddrRule rule;
  Cursor writeCursor;

 public:
#ifdef MAMEFONT_DEBUG
  Cursor dbgDumpCursor;
  Operator *dbgOpLog = nullptr;
  uint8_t lastInstByte1 = 0;
#endif

  StateMachine(const Font &font) {
    this->flags = font.flags();
    this->lut = font.blob + font.lutOffset();
    this->bytecode = font.blob + font.microCodeOffset();
    this->glyphHeight = font.glyphHeight();

#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    bool verticalFrag = false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    bool verticalFrag = true;
#else
    bool verticalFrag = FONT_FLAG_VERTICAL_FRAGMENT::read(flags);
#endif

    if (verticalFrag) {
      rule.lanesPerGlyph = (glyphHeight + 7) / 8;
#ifndef MAMEFONT_VERTICAL_FRAGMENT_ONLY
      rule.fragStride = 1;
#endif
    } else {
      rule.fragsPerLane = glyphHeight;
#ifndef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
      rule.laneStride = 1;
#endif
    }
  }

  Status run(Glyph &glyph, const GlyphBuffer &buff) {
    buffData = buff.data;

#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    bool verticalFrag = false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    bool verticalFrag = true;
#else
    bool verticalFrag = FONT_FLAG_VERTICAL_FRAGMENT::read(flags);
#endif

    int8_t glyphWidth = glyph.width();
    if (verticalFrag) {
      rule.fragsPerLane = glyphWidth;
#ifndef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
      rule.laneStride = buff.stride;
#endif
    } else {
#ifndef MAMEFONT_VERTICAL_FRAGMENT_ONLY
      rule.fragStride = buff.stride;
#endif
      rule.lanesPerGlyph = (glyphWidth + 7) / 8;
    }
    numLanesToGlyphEnd = rule.lanesPerGlyph;

    writeCursor.reset();

    lastFragment = 0x00;
    programCounter = glyph.entryPoint();

#ifdef MAMEFONT_DEBUG
    dbgDumpCursor = writeCursor;
    if (dbgOpLog) delete[] dbgOpLog;
    dbgOpLog = new Operator[buff.stride * glyphHeight];
    memset(dbgOpLog, 0, buff.stride * glyphHeight * sizeof(Operator));
    printf("  entryPoint    : %d\n", programCounter);
    printf("  glyphWidth    : %d\n", glyphWidth);
    printf("  fragsPerLane  : %d\n", rule.fragsPerLane);
    printf("  lanesPerGlyph : %d\n", rule.lanesPerGlyph);
    printf("  fragStride    : %d\n", rule.fragStride);
    printf("  laneStride    : %d\n", rule.laneStride);
#endif

    while (numLanesToGlyphEnd > 0) {
      uint8_t inst = mamefont_readBlobU8(bytecode + (programCounter++));
#ifdef MAMEFONT_DEBUG
      lastInstByte1 = inst;
#endif

      if ((inst & 0x80) == 0) {
        if ((inst & 0x40) == 0) {
          // 0x00-3F
          SFT(inst);
        } else if (((inst & 0x20) == 0) || (((inst & 0x07) != 0))) {
          // 0x40-7F except 0x60, 0x68, 0x70, 0x78
          if (inst == 0x40) {
#ifdef MAMEFONT_NO_CPX
            return Status::UNKNOWN_OPCODE;
#else
            CPX(inst);
#endif
          } else {
            CPY(inst);
          }
        } else if ((inst & 0x10) == 0) {
          // 0x60, 0x68
          if ((inst & 0x08) == 0) {
            LDI(inst);
          } else {
#ifdef MAMEFONT_NO_SFI
            return Status::UNKNOWN_OPCODE;
#else
            SFI(inst);
#endif
          }
        } else {
          // 0x70, 0x78
          return Status::UNKNOWN_OPCODE;
        }
      } else {
        if ((inst & 0x40) == 0) {
          // 0x80-BF
          LUP(inst);
        } else if ((inst & 0x20) == 0) {
          // 0xC0-DF
          LUD(inst);
        } else if ((inst & 0x10) == 0) {
          // 0xE0-EF
          RPT(inst);
        } else {
          // 0xF0-FE
          if (inst == 0xFF) {
            return Status::ABORTED_BY_ABO;
          } else {
            XOR(inst);
          }
        }
      }
    }
    return Status::SUCCESS;
  }

 private:
#ifdef MAMEFONT_DEBUG

#define MAMEFONT_BEFORE_OP(opr, inst_size, fmt, ...)                         \
  do {                                                                       \
    dbgDumpCursor = writeCursor;                                             \
    dbgOpLog[dbgDumpCursor.offset] = opr;                                    \
    char logBuff[128];                                                       \
    char *logPtr = logBuff;                                                  \
    char *logEnd = logBuff + sizeof(logBuff);                                \
    const int pc = programCounter - 1;                                       \
    logPtr += snprintf(logPtr, logEnd - logPtr, "    pc=%5d, ofst=%4d,", pc, \
                       (int)dbgDumpCursor.offset);                           \
    for (int i = 0; i < 4; i++) {                                            \
      if (i < (inst_size)) {                                                 \
        logPtr += snprintf(logPtr, logEnd - logPtr, " %02X",                 \
                           mamefont_readBlobU8(bytecode + (pc + i)));        \
      } else {                                                               \
        logPtr += snprintf(logPtr, logEnd - logPtr, "   ");                  \
      }                                                                      \
    }                                                                        \
    logPtr += snprintf(logPtr, logEnd - logPtr, "%-4s", getMnemonic(opr));   \
    logPtr += snprintf(logPtr, logEnd - logPtr, (fmt), ##__VA_ARGS__);       \
    printf("%-64s", logBuff);                                                \
    if (logPtr - logBuff > 64) {                                             \
      printf("\n");                                                          \
      for (int i = 0; i < 64; i++) printf(" ");                              \
    }                                                                        \
    printf("--> ");                                                          \
  } while (false)

#define MAMEFONT_AFTER_OP(len)                      \
  do {                                              \
    for (int i = 0; i < (len); i++) {               \
      if (i % 16 == 0 && i > 0) {                   \
        for (int j = 0; j < 64 + 4; j++) {          \
          printf(" ");                              \
        }                                           \
      }                                             \
      printf("%02X ", readPostIncr(dbgDumpCursor)); \
      if ((i + 1) % 16 == 0 || (i + 1) == (len)) {  \
        printf("\n");                               \
      }                                             \
    }                                               \
  } while (false)

#else

#define MAMEFONT_BEFORE_OP(op, fmt, ...) \
  do {                                   \
  } while (false)

#define MAMEFONT_AFTER_OP(len) \
  do {                         \
  } while (false)

#endif

  MAMEFONT_ALWAYS_INLINE
  void LUP(uint8_t inst) {
    uint8_t index = LUP_INDEX::read(inst);

    MAMEFONT_BEFORE_OP(Operator::LUP, 1, "(idx=%d)", (int)index);

    write(mamefont_readBlobU8(lut + index));

    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_ALWAYS_INLINE
  void LUD(uint8_t inst) {
    uint8_t index = LUD_INDEX::read(inst);
    bool step = LUD_STEP::read(inst);

    MAMEFONT_BEFORE_OP(Operator::LUD, 1, "(idx=%d, step=%d)", (int)index,
                       (step ? 1 : 0));

    const fragment_t *ptr = lut + index;
    write(mamefont_readBlobU8(ptr));
    if (step) ptr++;
    write(mamefont_readBlobU8(ptr));

    MAMEFONT_AFTER_OP(2);
  }

  MAMEFONT_ALWAYS_INLINE
  void SFT(uint8_t inst) {
    uint8_t size = SFT_SIZE::read(inst);
    uint8_t rpt = SFT_REPEAT_COUNT::read(inst);
    uint8_t flags = inst & (SFT_RIGHT::MASK | SFT_POST_SET::MASK);

    MAMEFONT_BEFORE_OP(Operator::SFT, 1, "(dir=%c, postOp=%c, size=%d, rpt=%d)",
                       (SFT_RIGHT::read(flags) ? 'R' : 'L'),
                       (SFT_POST_SET::read(flags) ? 'S' : 'C'), (int)size,
                       (int)rpt);

    shiftCore(flags, size, rpt, 1);

    MAMEFONT_AFTER_OP(rpt);
  }

#ifndef MAMEFONT_NO_SFI
  MAMEFONT_ALWAYS_INLINE
  void SFI(uint8_t inst) {
    uint8_t byte2 = mamefont_readBlobU8(bytecode + (programCounter + 0));
    uint8_t rpt = SFI_REPEAT_COUNT::read(byte2);
    uint8_t period = SFI_PERIOD::read(byte2);
    uint8_t flags =
        byte2 & (SFI_PRE_SHIFT::MASK | SFI_RIGHT::MASK | SFI_POST_SET::MASK);

    MAMEFONT_BEFORE_OP(Operator::SFI, 1,
                       "(dir=%c, period=%d, shift1st=%d, postOp=%c, rpt=%d)",
                       (SFI_RIGHT::read(flags) ? 'R' : 'L'), (int)period,
                       (SFI_PRE_SHIFT::read(flags) ? 1 : 0),
                       (SFI_POST_SET::read(flags) ? 'S' : 'C'), (int)rpt);

    shiftCore(flags, 1, rpt, period);

    MAMEFONT_AFTER_OP(rpt * period + (SFI_PRE_SHIFT::read(flags) ? 1 : 0));

    programCounter += 1;
  }
#endif

#ifndef MAMEFONT_NO_SFI
  MAMEFONT_NOINLINE
#endif
  void shiftCore(uint8_t flags, uint8_t size, uint8_t rpt, uint8_t period) {
    bool right = SFT_RIGHT::read(flags);
    fragment_t modifier = getRightMask(right ? (8 - size) : size);
    if (right) modifier = ~modifier;

    bool postSet = SFT_POST_SET::read(flags);
    if (!postSet) modifier = ~modifier;

    int8_t timer;
    if (SFI_PRE_SHIFT::read(flags)) {
      rpt++;
      timer = 1;
    } else {
      timer = period;
    }

    do {
      if (--timer == 0) {
        rpt--;
        timer = period;
        if (right) {
          lastFragment >>= size;
        } else {
          lastFragment <<= size;
        }
        if (postSet) {
          lastFragment |= modifier;
        } else {
          lastFragment &= modifier;
        }
      }
      write(lastFragment);
    } while (rpt != 0);
  }

  MAMEFONT_ALWAYS_INLINE
  void CPY(uint8_t inst) {
    uint8_t offset = CPY_OFFSET::read(inst);
    uint8_t length = CPY_LENGTH::read(inst);
    bool reverse = CPY_BYTE_REVERSE::read(inst);

    MAMEFONT_BEFORE_OP(Operator::CPY, 1, "(ofst=%d, len=%d, rev=%d)",
                       (int)offset, (int)length, (reverse ? 1 : 0));

    uint8_t flags;
    if (reverse) {
      flags = CPX_BYTE_REVERSE::MASK;
    } else {
      flags = 0;
      offset += length;
    }
    copyCore(flags, -offset, length);

    MAMEFONT_AFTER_OP(length);
  }

#ifndef MAMEFONT_NO_CPX
  MAMEFONT_ALWAYS_INLINE
  void CPX(uint8_t inst) {
    uint8_t byte2 = mamefont_readBlobU8(bytecode + (programCounter + 0));
    uint8_t byte3 = mamefont_readBlobU8(bytecode + (programCounter + 1));
    uint8_t flags = byte3 & (CPX_BYTE_REVERSE::MASK | CPX_BIT_REVERSE::MASK |
                             CPX_INVERSE::MASK);
    uint8_t length = (byte3 & CPX_LENGTH_MASK) + CPX_LENGTH_BIAS;
    frag_index_t offset = byte2 | ((CPX_OFFSET_H::read(byte3)) << 8);

    MAMEFONT_BEFORE_OP(
        Operator::CPX, 3, "(ofst=%d, len=%d, byteRev=%d, bitRev=%d, inv=%d)",
        (int)offset, (int)length, (int)CPX_BYTE_REVERSE::read(flags),
        (int)CPX_BIT_REVERSE::read(flags), (int)CPX_INVERSE::read(flags));

    if (CPX_BYTE_REVERSE::read(flags)) {
      offset -= length;
    }
    copyCore(flags, -offset, length);

    MAMEFONT_AFTER_OP(length);

    programCounter += 2;
  }
#endif

#ifndef MAMEFONT_NO_CPX
  MAMEFONT_NOINLINE
#endif
  void copyCore(uint8_t flags, frag_index_t offset, uint8_t length) {
    bool byteReverse = CPX_BYTE_REVERSE::read(flags);

    Cursor readCursor = writeCursor;
    readCursor.add(rule, offset);
    for (int8_t i = length; i != 0; i--) {
      int idx;
      fragment_t frag =
          byteReverse ? readPreDecr(readCursor) : readPostIncr(readCursor);
#ifndef MAMEFONT_NO_CPX
      if (CPX_BIT_REVERSE::read(flags)) frag = reverseBits(frag);
      if (CPX_INVERSE::read(flags)) frag = ~frag;
#endif
      write(frag);
    }
  }

  MAMEFONT_ALWAYS_INLINE void LDI(uint8_t inst) {
    fragment_t frag = mamefont_readBlobU8(bytecode + programCounter);
    MAMEFONT_BEFORE_OP(Operator::LDI, 2, "(frag=0x%02X)", frag);
    write(frag);
    MAMEFONT_AFTER_OP(1);
    programCounter += 1;
  }

  MAMEFONT_ALWAYS_INLINE
  void RPT(uint8_t inst) {
    uint8_t repeatCount = RPT_REPEAT_COUNT::read(inst);
    MAMEFONT_BEFORE_OP(Operator::RPT, 1, "(rpt=%d)", repeatCount);
    for (uint8_t i = repeatCount; i != 0; i--) {
      write(lastFragment);
    }
    MAMEFONT_AFTER_OP(repeatCount);
  }

  MAMEFONT_ALWAYS_INLINE
  void XOR(uint8_t inst) {
    uint8_t mask = XOR_WIDTH_2BIT::read(inst) ? 0x03 : 0x01;
    mask <<= XOR_POS::read(inst);
    MAMEFONT_BEFORE_OP(Operator::XOR, 1, "(mask=0x%02X)", mask);
    write(lastFragment ^ mask);
    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_NOINLINE
  void write(uint8_t value) {
    buffData[writeCursor.postIncr(rule)] = value;
    lastFragment = value;
    if (writeCursor.fragIndex == 0) {
      numLanesToGlyphEnd--;
    }
  }

  MAMEFONT_NOINLINE
  uint8_t readPostIncr(Cursor &cursor) const {
    frag_index_t laneOfst = cursor.laneOffset;
    frag_index_t ofst = cursor.postIncr(rule);
    if (laneOfst < 0 || ofst < 0) return 0;
    return buffData[ofst];
  }

  MAMEFONT_NOINLINE
  uint8_t readPreDecr(Cursor &cursor) const {
    frag_index_t ofst = cursor.preDecr(rule);
    if (cursor.laneOffset < 0 || ofst < 0) return 0;
    return buffData[ofst];
  }
};

Status extractGlyph(const Font &font, uint8_t c, const GlyphBuffer &buff,
                    GlyphDimensions *dims = nullptr);

}  // namespace mamefont
