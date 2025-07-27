#pragma once

#include "mamefont/mamefont_common.hpp"

namespace mamefont {

static uint8_t reverseBits(uint8_t b) {
  b = ((b & 0x55) << 1) | ((b & 0xaa) >> 1);
  b = ((b & 0x33) << 2) | ((b & 0xcc) >> 2);
  b = (b << 4) | (b >> 4);
  return b;
}

struct Glyph {
  const uint8_t *blob;
  bool isShrinked;

  Glyph() : blob(nullptr), isShrinked(false) {}
  Glyph(const uint8_t *data, bool isShrinked)
      : blob(data), isShrinked(isShrinked) {}

  MAMEFONT_ALWAYS_INLINE int16_t entryPoint() const {
    if (isShrinked) {
      return static_cast<int16_t>(blob[0]) << 1;
    } else {
      return reinterpret_cast<const int16_t *>(blob)[0];
    }
  }

  MAMEFONT_ALWAYS_INLINE bool isValid() const {
    return !(blob[0] == 0xff && blob[1] == 0xff);
  }

  MAMEFONT_ALWAYS_INLINE uint8_t width() const {
    if (isShrinked) {
      return (blob[1] & 0x0f) + 1;
    } else {
      return (blob[2] & 0x3f) + 1;
    }
  }

  MAMEFONT_ALWAYS_INLINE uint8_t xAdvance() const {
    if (isShrinked) {
      return ((blob[1] >> 4) & 0x0f) + 1;
    } else {
      return (blob[3] & 0x3f) + 1;
    }
  }
};

class Font {
 public:
  const uint8_t *blob;

  Font(const uint8_t *blob) : blob(blob) {}

  MAMEFONT_ALWAYS_INLINE FontFlags flags() const {
    return static_cast<FontFlags>(blob[OFST_FONT_FLAGS]);
  }

  MAMEFONT_ALWAYS_INLINE bool isShrinkedGlyphTable() const {
    return !!(flags() & FontFlags::FONT_FLAG_SHRINKED_GLYPH_TABLE);
  }

  MAMEFONT_ALWAYS_INLINE uint8_t glyphTableEntrySize() const {
    return isShrinkedGlyphTable() ? 2 : 4;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t firstCode() const {
    return blob[OFST_FIRST_CODE];
  }

  MAMEFONT_ALWAYS_INLINE uint8_t numGlyphs() const {
    return blob[OFST_GLYPH_TABLE_LEN] + 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t lastCode() const {
    return firstCode() + numGlyphs() - 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t lutSize() const {
    return blob[OFST_LUT_SIZE] + 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t fontHeight() const {
    return (blob[OFST_FONT_DIMENSION_0] & 0x3f) + 1;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t maxGlyphWidth() const {
    return (blob[OFST_FONT_DIMENSION_2] & 0x3f) + 1;
  }

  MAMEFONT_ALWAYS_INLINE const bool verticalFragment() const {
#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    return false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    return true;
#else
    return blob[OFST_FONT_FLAGS] & FontFlags::FONT_FLAG_VERTICAL_FRAGMENT;
#endif
  }

  MAMEFONT_ALWAYS_INLINE const Glyph glyphTableEntry(uint8_t index) const {
    const uint8_t *ptr = blob + OFST_GLYPH_TABLE;
    ptr += isShrinkedGlyphTable() ? (index << 1) : (index << 2);
    return Glyph(ptr, isShrinkedGlyphTable());
  }

  Status getGlyph(uint8_t c, Glyph *glyph) const {
    uint8_t first = firstCode();
    uint8_t last = lastCode();
    if (c < first || last < c) return Status::CHAR_CODE_OUT_OF_RANGE;
    uint8_t index = c - first;

    auto g = glyphTableEntry(index);
    if (glyph) *glyph = g;

    return g.isValid() ? Status::SUCCESS : Status::GLYPH_NOT_DEFINED;
  }

  MAMEFONT_ALWAYS_INLINE int16_t lutOffset() const {
    if (isShrinkedGlyphTable()) {
      return OFST_GLYPH_TABLE + (numGlyphs() << 1);
    } else {
      return OFST_GLYPH_TABLE + (numGlyphs() << 2);
    }
  }

  MAMEFONT_ALWAYS_INLINE int16_t microCodeOffset() const {
    return lutOffset() + lutSize();
  }

  MAMEFONT_ALWAYS_INLINE const uint8_t *getEntryPoint(
      const Glyph &glyph) const {
    return blob + microCodeOffset() + glyph.entryPoint();
  }

  int16_t getRequiredGlyphBufferSize(const Glyph *glyph, int8_t *stride) const {
    int8_t w = glyph->width();
    int8_t h = fontHeight();
    if (verticalFragment()) {
      *stride = w;
      return w * ((h + 7) / 8);
    } else {
      *stride = ((w + 7) / 8);
      return *stride * h;
    }
  }

  int16_t getRequiredGlyphBufferSize(int8_t *stride) const {
    int8_t w = maxGlyphWidth();
    int8_t h = fontHeight();
    if (verticalFragment()) {
      *stride = w;
      return w * ((h + 7) / 8);
    } else {
      *stride = ((w + 7) / 8);
      return *stride * h;
    }
  }
};

struct GlyphBuffer {
  uint8_t *data;
  int16_t stride;
};

class Renderer {
 private:
  struct AddrRule {
    int8_t lanesPerGlyph;     // Number of lanes per glyph
    int8_t fragsPerLane;      // Number of fragments per lane
    frag_index_t laneStride;  // Stride to next lane in bytes
    frag_index_t fragStride;  // Stride to next fragment in bytes
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

    void add(const AddrRule &rule, frag_index_t delta) {
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

    frag_index_t postIncr(const AddrRule &rule) {
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

    frag_index_t preDecr(const AddrRule &rule) {
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

  FontFlags flags;
  const fragment_t *lut;
  const uint8_t *bytecode;
  uint8_t fontHeight;

  uint8_t *buffData;

  int8_t numLanesToGlyphEnd = 0;

  prog_cntr_t programCounter = 0;
  fragment_t lastFragment = 0;

  AddrRule rule;
  Cursor writeCursor;

 public:
#ifdef MAMEFONT_STM_DEBUG
  Cursor dbgDumpCursor;
  Operator *dbgOpLog = nullptr;
#endif

  Renderer(const Font &font) {
    this->flags = font.flags();
    this->lut = font.blob + font.lutOffset();
    this->bytecode = font.blob + font.microCodeOffset();
    this->fontHeight = font.fontHeight();

#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    bool verticalFrag = false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    bool verticalFrag = true;
#else
    bool verticalFrag = flags & FontFlags::FONT_FLAG_VERTICAL_FRAGMENT;
#endif

    if (verticalFrag) {
      rule.lanesPerGlyph = (fontHeight + 7) / 8;
      rule.fragStride = 1;
    } else {
      rule.fragsPerLane = fontHeight;
      rule.laneStride = 1;
    }
  }

  Status render(Glyph &glyph, const GlyphBuffer &buff) {
    buffData = buff.data;

#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    bool verticalFrag = false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    bool verticalFrag = true;
#else
    bool verticalFrag = flags & FontFlags::FONT_FLAG_VERTICAL_FRAGMENT;
#endif

    int8_t glyphWidth = glyph.width();
    if (verticalFrag) {
      rule.fragsPerLane = glyphWidth;
      rule.laneStride = buff.stride;
    } else {
      rule.fragStride = buff.stride;
      rule.lanesPerGlyph = (glyphWidth + 7) / 8;
    }
    numLanesToGlyphEnd = rule.lanesPerGlyph;

    writeCursor.reset();

    lastFragment = 0x00;
    programCounter = glyph.entryPoint();

#ifdef MAMEFONT_STM_DEBUG
    dbgDumpCursor = writeCursor;
    if (dbgOpLog) delete[] dbgOpLog;
    dbgOpLog = new Operator[buff.stride * fontHeight];
    memset(dbgOpLog, 0, buff.stride * fontHeight * sizeof(Operator));
    printf("  entryPoint    : %d\n", programCounter);
    printf("  glyphWidth    : %d\n", glyphWidth);
    printf("  fragsPerLane  : %d\n", rule.fragsPerLane);
    printf("  lanesPerGlyph : %d\n", rule.lanesPerGlyph);
    printf("  fragStride    : %d\n", rule.fragStride);
    printf("  laneStride    : %d\n", rule.laneStride);
#endif

    while (numLanesToGlyphEnd > 0) {
      uint8_t inst = bytecode[programCounter++];

      switch (inst & 0xf0) {
        case 0x00:
        case 0x10:
        case 0x20:
        case 0x30:
          LUP(inst);
          break;

        case 0x40:
        case 0x50:
          LUD(inst);
          break;

        case 0x60:
          RPT(inst);
          break;

        case 0x70:
          if (inst == 0x7f) {
            return Status::UNKNOWN_OPCODE;
          } else {
            XOR(inst);
          }
          break;

        case 0x80:
        case 0x90:
        case 0xa0:
        case 0xb0:
          SFT(inst);
          break;

        case 0xc0:
        case 0xd0:
        case 0xe0:
        case 0xf0:
          if (inst == 0xc0) {
            LDI(inst);
          } else if (inst == 0xe0) {
            CPX(inst);
          } else if (inst == 0xe8 || inst == 0xf0 || inst == 0xf8) {
            return Status::UNKNOWN_OPCODE;
          } else {
            CPY(inst);
          }
          break;

        default:
          return Status::UNKNOWN_OPCODE;
      }
    }
    return Status::SUCCESS;
  }

 private:
#ifdef MAMEFONT_STM_DEBUG

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
        logPtr +=                                                            \
            snprintf(logPtr, logEnd - logPtr, " %02X", bytecode[pc + i]);    \
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

  MAMEFONT_ALWAYS_INLINE void LUP(uint8_t inst) {
    uint8_t index = inst & 0x3f;
    fragment_t byte = lut[index];
    MAMEFONT_BEFORE_OP(Operator::LUP, 1, "(idx=%d)", index);
    write(byte);
    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_ALWAYS_INLINE void LUD(uint8_t inst) {
    uint8_t index = inst & 0x0f;
    uint8_t step = (inst >> 4) & 0x1;
    MAMEFONT_BEFORE_OP(Operator::LUD, 1, "(idx=%d, step=%d)", index, step);
    write(lut[index]);
    write(lut[index + step]);
    MAMEFONT_AFTER_OP(2);
  }

  MAMEFONT_ALWAYS_INLINE void SFT(uint8_t inst) {
    uint8_t dir = inst & 0x20;
    uint8_t postOp = inst & 0x10;
    uint8_t size = ((inst >> 2) & 0x3) + 1;
    uint8_t rptCount = (inst & 0x03) + 1;

    MAMEFONT_BEFORE_OP(Operator::SFT, 1, "(dir=%d, postOp=%d, size=%d, rpt=%d)",
                       dir, postOp, size, rptCount);

    fragment_t modifier = (1 << size) - 1;
    if (dir != 0) modifier <<= (8 - size);
    if (postOp == 0) modifier = ~modifier;
    for (int8_t i = rptCount; i != 0; i--) {
      if (dir == 0) {
        lastFragment <<= size;
      } else {
        lastFragment >>= size;
      }
      if (postOp) {
        lastFragment |= modifier;
      } else {
        lastFragment &= modifier;
      }
      write(lastFragment);
    }

    MAMEFONT_AFTER_OP(rptCount);
  }

  MAMEFONT_ALWAYS_INLINE void CPY(uint8_t inst) {
    uint8_t offset = (inst >> 3) & 0x3;
    uint8_t length = (inst & 0x07) + 1;
    uint8_t reverse = (inst & 0x20) != 0;

    MAMEFONT_BEFORE_OP(Operator::CPY, 1, "(ofst=%d, len=%d, rev=%d)", offset,
                       length, reverse);

    if (reverse) {
      Cursor readCursor = writeCursor;
      readCursor.add(rule, -offset);
      for (int8_t i = length; i != 0; i--) {
        write(readPreDecr(readCursor));
      }
    } else {
      Cursor readCursor = writeCursor;
      readCursor.add(rule, -length - offset);
      for (int8_t i = length; i != 0; i--) {
        write(readPostIncr(readCursor));
      }
    }

    MAMEFONT_AFTER_OP(length);
  }

  MAMEFONT_ALWAYS_INLINE void CPX(uint8_t inst) {
    uint8_t byte2 = bytecode[programCounter + 0];
    uint8_t byte3 = bytecode[programCounter + 1];

    bool bitReverse = byte3 & CPX_MASK_BIT_REVERSAL;
    bool byteReverse = byte3 & CPX_MASK_BYTE_REVERSAL;
    bool inverse = byte3 & CPX_MASK_INVERSE;

    uint8_t length = ((byte3 & CPX_MASK_LENGTH) >> CPX_POS_LENGTH) * 4 + 4;
    frag_index_t absOffset = byte2 | (((frag_index_t)(byte3 & CPX_MASK_ABS_OFFSET_H)) << 8);

    MAMEFONT_BEFORE_OP(Operator::CPX, 3,
                       "(ofst=%d, len=%d, bitRev=%d, byteRev=%d, inv=%d)",
                       (int)absOffset, (int)length, (int)bitReverse,
                       (int)byteReverse, (int)inverse);

    Cursor readCursor;
    readCursor.reset();

    if (byteReverse) {
      readCursor.add(rule, absOffset + length);
      for (int8_t i = length; i != 0; i--) {
        int idx;
        fragment_t frag = readPreDecr(readCursor);
        if (bitReverse) frag = reverseBits(frag);
        if (inverse) frag = ~frag;
        write(frag);
      }
    } else {
      readCursor.add(rule, absOffset);
      for (int8_t i = length; i != 0; i--) {
        fragment_t frag = readPostIncr(readCursor);
        if (bitReverse) frag = reverseBits(frag);
        if (inverse) frag = ~frag;
        write(frag);
      }
    }

    MAMEFONT_AFTER_OP(length);

    programCounter += 2;
  }

  MAMEFONT_ALWAYS_INLINE void LDI(uint8_t inst) {
    fragment_t frag = bytecode[programCounter];
    MAMEFONT_BEFORE_OP(Operator::LDI, 2, "(frag=0x%02X)", frag);
    write(frag);
    MAMEFONT_AFTER_OP(1);
    programCounter += 1;
  }

  MAMEFONT_ALWAYS_INLINE void RPT(uint8_t inst) {
    uint8_t repeatCount = (inst & 0x0f) + 1;
    MAMEFONT_BEFORE_OP(Operator::RPT, 1, "(rpt=%d)", repeatCount);
    for (int8_t i = repeatCount; i != 0; i--) {
      write(lastFragment);
    }
    MAMEFONT_AFTER_OP(repeatCount);
  }

  MAMEFONT_ALWAYS_INLINE void XOR(uint8_t inst) {
    uint8_t width = ((inst >> 3) & 0x01) + 1;
    uint8_t pos = inst & 0x07;
    uint8_t mask = (1 << width) - 1;
    MAMEFONT_BEFORE_OP(Operator::XOR, 1, "(width=%d, pos=%d)", width, pos);
    write(lastFragment ^ (mask << pos));
    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_ALWAYS_INLINE void write(uint8_t value) {
    buffData[writeCursor.postIncr(rule)] = value;
    lastFragment = value;
    if (writeCursor.fragIndex == 0) {
      numLanesToGlyphEnd--;
    }
  }

  MAMEFONT_ALWAYS_INLINE uint8_t readPostIncr(Cursor &cursor) const {
    frag_index_t laneOfst = cursor.laneOffset;
    frag_index_t ofst = cursor.postIncr(rule);
    if (laneOfst < 0 || ofst < 0) return 0;
    return buffData[ofst];
  }

  MAMEFONT_ALWAYS_INLINE uint8_t readPreDecr(Cursor &cursor) const {
    frag_index_t ofst = cursor.preDecr(rule);
    if (cursor.laneOffset < 0 || ofst < 0) return 0;
    return buffData[ofst];
  }
};

Status drawChar(const Font &font, uint8_t c, const GlyphBuffer &buff,
                int8_t *glyphWidth = nullptr, int8_t *xAdvance = nullptr);

}  // namespace mamefont
