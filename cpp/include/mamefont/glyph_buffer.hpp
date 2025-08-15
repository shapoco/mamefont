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

  MAMEFONT_INLINE uint8_t getPixel(const Font &font, const Glyph &glyph,
                                   int8_t x, int8_t y) const {
    if (!data || !glyph.isValid()) return 0;
    if (x < 0 || glyph.glyphWidth <= x) return 0;
    if (y < 0 || font.glyphHeight() <= y) return 0;

    bool bpp1 = (font.bitsPerPixel() == PixelFormat::BW_1BIT);
    uint8_t iPixel;
    frag_index_t offset;
    if (font.verticalFragment()) {
      offset = y >> 2;
      if (bpp1) offset >>= 1;
      offset = offset * stride + x;
      iPixel = y & (bpp1 ? 7 : 3);
    } else {
      offset = x >> 2;
      if (bpp1) offset >>= 1;
      offset = offset + y * stride;
      iPixel = x & (bpp1 ? 7 : 3);
    }
    if (font.farPixelFirst()) {
      iPixel = (bpp1 ? 7 : 3) - iPixel;
    }
    if (bpp1) {
      return (data[offset] >> iPixel) & 1;
    }
    else {
      return (data[offset] >> (iPixel * 2)) & 0x03;
    }
  }
};

}  // namespace mamefont