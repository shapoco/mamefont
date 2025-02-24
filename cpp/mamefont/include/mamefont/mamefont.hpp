#pragma once

#include <stdint.h>

namespace mfnt {

static constexpr uint16_t GLYPH_OFFSET_DUMMY = 0xffffu;
static constexpr uint8_t SEGMENT_HEIGHT = 8;

enum class Status {
  SUCCESS = 0,
  CHAR_CODE_OUT_OF_RANGE,
  GLYPH_NOT_DEFINED,
  UNKNOWN_OPCODE,
};

struct GlyphInfo {
  uint8_t width;
  const uint8_t *data;
};

struct ExtractContext {
  uint8_t *buff;
  uint16_t start;
  uint16_t end;
  uint16_t pos = 0;
  uint8_t last = 0;
  ExtractContext(uint8_t *buff, uint16_t start = 0, uint16_t end = -1)
      : buff(buff), start(start), end(end) {}
  void write(uint8_t value) {
    if (start <= pos && pos < end) {
      buff[pos++] = value;
    }
  }
};

static inline void shiftOp(ExtractContext *ctx, uint8_t inst) {
  uint8_t last = ctx->last;
  uint8_t shift_dir = inst & 0x20;
  uint8_t post_op = inst & 0x10;
  uint8_t len = (inst & 0x07) + 1;
  for (int8_t i = len; i != 0; i--) {
    uint8_t shift_size = (inst >> 3) & 0x1;
    do {
      if (shift_dir == 0) {
        last = (last << 1) & 0xfe;
        if (post_op) last |= 0x01;
      }
      else {
        last = (last >> 1) & 0x7f;
        if (post_op) last |= 0x80;
      }
    } while (shift_size-- != 0);
    ctx->write(last);
  }
  ctx->last = last;
}

static inline void copyOp(ExtractContext *ctx, uint8_t inst) {
  uint8_t offset = (inst >> 4) & 0x3;
  uint8_t len = (inst & 0x0f) + 1;
  uint8_t *src = ctx->buff + ctx->pos - len - offset;
  for (int i = len; i != 0; i--) {
    ctx->write(*(src++));
  }
  ctx->last = ctx->buff[ctx->pos - 1];
}

static inline void repeatOp(ExtractContext *ctx, uint8_t inst) {
  uint8_t len = (inst & 0x0f) + 1;
  for (int i = len; i != 0; i--) {
    ctx->write(ctx->last);
  }
}

static inline void xorOp(ExtractContext *ctx, uint8_t inst) {
  uint8_t mask = (inst & 0x08) ? 0x03 : 0x01;
  uint8_t bit = inst & 0x07;
  ctx->last = ctx->buff[ctx->pos++] = ctx->last ^ (mask << bit);
}

class Font {
public:
  const uint8_t height;
  const uint8_t rows;
  const uint8_t numChars;
  const uint8_t codeOffset;
  const uint8_t flags;
  const uint16_t *charTable;
  const uint8_t *segTable;
  const uint8_t *glyphData;

  Font(
    const uint8_t height, 
    const uint8_t numChars,
    const uint8_t codeOffset, 
    const uint8_t flags, 
    const uint16_t *charTable, 
    const uint8_t *segTable, 
    const uint8_t *glyphData
  ) : 
    height(height), 
    rows((height + SEGMENT_HEIGHT - 1) / SEGMENT_HEIGHT),
    numChars(numChars), 
    codeOffset(codeOffset),
    flags(flags),
    charTable(charTable), 
    segTable(segTable), 
    glyphData(glyphData) {}

  Status getGlyph(char c, GlyphInfo *glyph) const {
    if (c < codeOffset) return Status::CHAR_CODE_OUT_OF_RANGE;
    uint8_t index = c - codeOffset;
    if (index >= numChars) return Status::CHAR_CODE_OUT_OF_RANGE;
    uint16_t offset = charTable[index];
    if (offset == GLYPH_OFFSET_DUMMY) return Status::GLYPH_NOT_DEFINED;
    if (glyph) {
      const uint8_t *ptr = glyphData + offset;
      glyph->width = *ptr;
      glyph->data = ptr + 1;
    }
    return Status::SUCCESS;
  }

  uint8_t getMaxWidth() const {
    uint8_t maxWidth = 0;
    for (uint8_t i = 0; i < numChars; i++) {
      uint16_t offset = charTable[i];
      if (offset == GLYPH_OFFSET_DUMMY) continue;
      uint8_t width = glyphData[offset];
      if (width > maxWidth) maxWidth = width;
    }
    return maxWidth;
  }

  uint16_t getGlyphSizeInBytes(const GlyphInfo* glyph) const {
    return glyph->width * rows;
  }

  Status extractGlyph(const GlyphInfo *glyph, ExtractContext *ctx) const {
    if (ctx->end == 0xffff) {
      ctx->end = getGlyphSizeInBytes(glyph);
    }
    const uint8_t *rdPtr = glyph->data;
    while (ctx->pos < ctx->end) {
      uint8_t inst = *(rdPtr++);
      switch (inst & 0xf0) {
        case 0x00: // LD
        case 0x10: // LD
        case 0x20: // LD
        case 0x30: // LD
          ctx->last = ctx->buff[ctx->pos++] = segTable[inst & 0x3f];
          break;
        
        case 0x40: // SLC
        case 0x50: // SLS
        case 0x60: // SRC
        case 0x70: // SRS
          shiftOp(ctx, inst);
          break;
        
        case 0x80: // CPY
        case 0x90: // CPY
        case 0xa0: // CPY
        case 0xb0: // CPY
          copyOp(ctx, inst);
          break;
        
        case 0xc0: // REV
        case 0xd0: // REV
          return Status::UNKNOWN_OPCODE;
        
        case 0xe0: // RPT
          repeatOp(ctx, inst);
          break;

        case 0xf0: // XOR
          if (inst == 0xff) return Status::UNKNOWN_OPCODE;
          xorOp(ctx, inst);
          break;

        default:
          return Status::UNKNOWN_OPCODE;
      }
    }
    return Status::SUCCESS;
  }

  
};

}
