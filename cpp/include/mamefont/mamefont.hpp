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

  MAMEFONT_ALWAYS_INLINE void getDimensions(int8_t *width,
                                            int8_t *xSpacing) const {
    if (isShrinked) {
      uint8_t dim = mamefont_readBlobU8(blob + 1);
      if (width) *width = (dim & 0x0f) + 1;
      if (xSpacing) *xSpacing = ((dim >> 4) & 0x03);
    } else {
      if (width) *width = (mamefont_readBlobU8(blob + 2) & 0x3f) + 1;
      if (xSpacing) *xSpacing = (mamefont_readBlobU8(blob + 3) & 0x1f) - 16;
    }
  }

  MAMEFONT_ALWAYS_INLINE uint8_t width() const {
    int8_t ret;
    getDimensions(&ret, nullptr);
    return ret;
  }

  MAMEFONT_ALWAYS_INLINE uint8_t xSpacing() const {
    int8_t ret;
    getDimensions(nullptr, &ret);
    return ret;
  }
};

class Font {
 public:
  const uint8_t *blob;

  Font(const uint8_t *blob) : blob(blob) {}

  MAMEFONT_ALWAYS_INLINE FontFlags flags() const {
    return static_cast<FontFlags>(mamefont_readBlobU8(blob + OFST_FONT_FLAGS));
  }

  MAMEFONT_ALWAYS_INLINE bool isShrinkedGlyphTable() const {
    return !!(flags() & FontFlags::FONT_FLAG_SHRINKED_GLYPH_TABLE);
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

  MAMEFONT_ALWAYS_INLINE const bool verticalFragment() const {
#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    return false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    return true;
#else
    return mamefont_readBlobU8(blob + OFST_FONT_FLAGS) &
           FontFlags::FONT_FLAG_VERTICAL_FRAGMENT;
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

  int16_t getRequiredGlyphBufferSize(const Glyph *glyph, int8_t *stride) const {
    int8_t w = glyph->width();
    int8_t h = glyphHeight();
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
    int8_t h = glyphHeight();
    if (verticalFragment()) {
      *stride = w;
      return w * ((h + 7) / 8);
    } else {
      *stride = ((w + 7) / 8);
      return *stride * h;
    }
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

  Renderer(const Font &font) {
    this->flags = font.flags();
    this->lut = font.blob + font.lutOffset();
    this->bytecode = font.blob + font.microCodeOffset();
    this->glyphHeight = font.glyphHeight();

#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
    bool verticalFrag = false;
#elifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
    bool verticalFrag = true;
#else
    bool verticalFrag = flags & FontFlags::FONT_FLAG_VERTICAL_FRAGMENT;
#endif

    if (verticalFrag) {
      rule.lanesPerGlyph = (glyphHeight + 7) / 8;
      rule.fragStride = 1;
    } else {
      rule.fragsPerLane = glyphHeight;
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

      switch (inst & 0xF0) {
        case 0x00:
        case 0x10:
        case 0x20:
        case 0x30:
          SFT(inst);
          break;

        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
          LUP(inst);
          break;

        case 0xC0:
        case 0xD0:
          LUD(inst);
          break;

        case 0xE0:
          RPT(inst);
          break;

        case 0xF0:
          if (inst == 0xFF) {
            return Status::ABORTED_BY_ABO;
          } else {
            XOR(inst);
          }
          break;

        case 0x40:
        case 0x50:
        case 0x60:
        case 0x70:
          if (inst == 0x40) {
            CPX(inst);
          } else if (inst == 0x60) {
            LDI(inst);
          } else if (inst == 0x68 || inst == 0x70 || inst == 0x78) {
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

  MAMEFONT_ALWAYS_INLINE void LUP(uint8_t inst) {
    uint8_t index = inst & 0x3f;
    fragment_t byte = mamefont_readBlobU8(lut + index);
    MAMEFONT_BEFORE_OP(Operator::LUP, 1, "(idx=%d)", index);
    write(byte);
    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_ALWAYS_INLINE void LUD(uint8_t inst) {
    uint8_t index = inst & 0x0f;
    uint8_t step = (inst >> 4) & 0x1;
    MAMEFONT_BEFORE_OP(Operator::LUD, 1, "(idx=%d, step=%d)", index, step);
    write(mamefont_readBlobU8(lut + index));
    write(mamefont_readBlobU8(lut + index + step));
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
    uint8_t byte2 = mamefont_readBlobU8(bytecode + (programCounter + 0));
    uint8_t byte3 = mamefont_readBlobU8(bytecode + (programCounter + 1));

    bool bitReverse = byte3 & CPX_BIT_REVERSAL_MASK;
    bool byteReverse = byte3 & CPX_BYTE_REVERSAL_MASK;
    bool inverse = byte3 & CPX_INVERSE_MASK;

    uint8_t length = (byte3 & CPX_LENGTH_MASK) + CPX_LENGTH_BIAS;
    frag_index_t offset =
        byte2 | (((frag_index_t)(byte3 & CPX_OFFSET_H_MASK)) << 8);

    MAMEFONT_BEFORE_OP(Operator::CPX, 3,
                       "(ofst=%d, len=%d, bitRev=%d, byteRev=%d, inv=%d)",
                       (int)offset, (int)length, (int)bitReverse,
                       (int)byteReverse, (int)inverse);

    Cursor readCursor = writeCursor;
    if (byteReverse) {
      readCursor.add(rule, -offset + length);
      for (int8_t i = length; i != 0; i--) {
        int idx;
        fragment_t frag = readPreDecr(readCursor);
        if (bitReverse) frag = reverseBits(frag);
        if (inverse) frag = ~frag;
        write(frag);
      }
    } else {
      readCursor.add(rule, -offset);
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
    fragment_t frag = mamefont_readBlobU8(bytecode + programCounter);
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
