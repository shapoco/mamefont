#pragma once

#include <stdint.h>

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE __attribute__((always_inline))
#endif

namespace mfnt {

static constexpr uint16_t GLYPH_OFFSET_DUMMY = 0xffffu;
static constexpr uint8_t SEGMENT_HEIGHT = 8;
static constexpr uint8_t HISTORY_SIZE = 32;

#define MFNT_SUPPORT_RESUME (0)

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
  GlyphInfo *glyph;
  uint8_t *buff;
  uint16_t start;
  uint16_t end;
  uint16_t wrPos;
  uint8_t last;
#if MFNT_SUPPORT_RESUME
  uint8_t *history;
#endif
  const uint8_t *rdPtr;

  void init(GlyphInfo *glyph, uint8_t *buff, uint16_t start = 0,
            uint16_t end = 0xffff
#if MFNT_SUPPORT_RESUME
            ,
            uint8_t *history = nullptr
#endif
  ) {
    this->glyph = glyph;
    this->buff = buff;
    this->start = start;
    this->end = end;
    this->wrPos = 0;
    this->last = 0;
#if MFNT_SUPPORT_RESUME
    this->history = history;
#endif
    this->rdPtr = glyph->data;
  }

  void write(uint8_t value) {
    if (start <= wrPos && wrPos < end) {
      buff[wrPos++] = value;
    }
#if MFNT_SUPPORT_RESUME
    if (history) {
      history[wrPos & (HISTORY_SIZE - 1)] = value;
    }
#endif
    this->last = value;
  }

  uint8_t read(uint16_t pos) const {
#if MFNT_SUPPORT_RESUME
    if (history) {
      return history[pos & (HISTORY_SIZE - 1)];
    } else {
      return buff[pos];
    }
#else
    return buff[pos];
#endif
  }
};  // namespace mfnt

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
      } else {
        last = (last >> 1) & 0x7f;
        if (post_op) last |= 0x80;
      }
    } while (shift_size-- != 0);
    ctx->write(last);
  }
}

static inline void copyOp(ExtractContext *ctx, uint8_t inst) {
  uint8_t offset = (inst >> 4) & 0x3;
  uint8_t len = (inst & 0x0f) + 1;
  uint16_t rdPos = ctx->wrPos - len - offset;
  for (int i = len; i != 0; i--) {
    ctx->write(ctx->read(rdPos++));
  }
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
  ctx->write(ctx->last ^ (mask << bit));
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

  Font(const uint8_t height, const uint8_t numChars, const uint8_t codeOffset,
       const uint8_t flags, const uint16_t *charTable, const uint8_t *segTable,
       const uint8_t *glyphData)
      : height(height),
        rows((height + SEGMENT_HEIGHT - 1) / SEGMENT_HEIGHT),
        numChars(numChars),
        codeOffset(codeOffset),
        flags(flags),
        charTable(charTable),
        segTable(segTable),
        glyphData(glyphData) {}

  Status getGlyph(uint8_t c, GlyphInfo *glyph) const {
    uint8_t index = c;
    if (index < codeOffset) return Status::CHAR_CODE_OUT_OF_RANGE;
    index -= codeOffset;
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

  uint16_t getGlyphSizeInBytes(const GlyphInfo *glyph) const {
    return glyph->width * rows;
  }

  Status extractGlyph(ExtractContext *ctx) const {
    if (ctx->end == 0xffff) {
      ctx->end = getGlyphSizeInBytes(ctx->glyph);
    }
    while (ctx->wrPos < ctx->end) {
      uint8_t inst = *(ctx->rdPtr++);
      switch (inst & 0xf0) {
        case 0x00:  // LD
        case 0x10:  // LD
        case 0x20:  // LD
        case 0x30:  // LD
          ctx->last = ctx->buff[ctx->wrPos++] = segTable[inst & 0x3f];
          break;

        case 0x40:  // SLC
        case 0x50:  // SLS
        case 0x60:  // SRC
        case 0x70:  // SRS
          shiftOp(ctx, inst);
          break;

        case 0x80:  // CPY
        case 0x90:  // CPY
        case 0xa0:  // CPY
        case 0xb0:  // CPY
          copyOp(ctx, inst);
          break;

        case 0xc0:  // REV
        case 0xd0:  // REV
          return Status::UNKNOWN_OPCODE;

        case 0xe0:  // RPT
          repeatOp(ctx, inst);
          break;

        case 0xf0:  // XOR
          if (inst == 0xff) return Status::UNKNOWN_OPCODE;
          xorOp(ctx, inst);
          break;

        default:
          return Status::UNKNOWN_OPCODE;
      }
    }
    return Status::SUCCESS;
  }

  Status extractGlyph(uint8_t c, uint8_t *buff, uint8_t *width) const {
    GlyphInfo glyph;
    Status ret = getGlyph(c, &glyph);
    if (ret != Status::SUCCESS) return ret;
    ExtractContext ctx;
    ctx.init(&glyph, buff);
    if (width) *width = glyph.width;
    return extractGlyph(&ctx);
  }
};
}  // namespace mfnt
