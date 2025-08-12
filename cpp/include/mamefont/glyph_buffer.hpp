#pragma once

#include "mamefont/font.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

struct GlyphBuffer {
  uint8_t *data;
  frag_index_t stride;

  GlyphBuffer() : data(nullptr), stride(0) {}
  GlyphBuffer(uint8_t *data, frag_index_t stride)
      : data(data), stride(stride) {}

  MAMEFONT_INLINE bool getPixel(const Font &font, const Glyph &glyph, int8_t x,
                                int8_t y) const {
    if (!data || !glyph.isValid()) return false;
    if (x < 0 || glyph.glyphWidth <= x) return false;
    if (y < 0 || font.glyphHeight() <= y) return false;

    uint8_t ibit;
    frag_index_t offset;
    if (font.verticalFragment()) {
      offset = y / 8 * stride + x;
      ibit = y & 0x07;
    } else {
      offset = x / 8 + y * stride;
      ibit = x & 0x07;
    }
    if (font.msb1st()) {
      ibit = 7 - ibit;
    }
    uint8_t mask = 1 << ibit;
    return (data[offset] & mask) != 0;
  }
};

}  // namespace mamefont