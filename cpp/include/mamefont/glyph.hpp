#pragma once

#include "mamefont/bit_field.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

struct Glyph {
 public:
  using Valid = BitFlag<0, 0>;
  using FarPixelFirst = BitFlag<0, 2>;
  using VerticalFragment = BitFlag<0, 3>;
  using FragFormat = BitField<uint8_t, uint8_t, 0, 4, 2>;
  using UseAltTop = BitFlag<0, 6>;
  using UseAltBottom = BitFlag<0, 7>;

  uint16_t entryPoint;
  uint8_t flags;
  uint8_t glyphWidth;
  uint8_t glyphHeight;
  int8_t xSpace;
  uint8_t xStepBack;
  uint8_t yOffset;

  frag_t *data;

  Glyph() = default;
  Glyph(uint8_t *data) : data(data) {}

  MAMEFONT_INLINE bool isValid() const { return Valid::read(flags); }

  MAMEFONT_INLINE bool verticalFragment() const {
#ifdef MAMEFONT_HORI_FRAG_ONLY
    return false;
#elif defined(MAMEFONT_VERT_FRAG_ONLY)
    return true;
#else
    return VerticalFragment::read(flags);
#endif
  }

  MAMEFONT_INLINE bool farPixelFirst() const {
#ifdef MAMEFONT_NEAR1ST_ONLY
    return false;
#elif defined(MAMEFONT_FAR1ST_ONLY)
    return true;
#else
    return FarPixelFirst::read(flags);
#endif
  }

  MAMEFONT_INLINE PixelFormat pixelFormat() const {
#if defined(MAMEFONT_1BPP_ONLY)
    return PixelFormat::BW_1BIT;
#elif defined(MAMEFONT_2BPP_ONLY)
    return PixelFormat::GRAY_2BIT;
#else
    return FragFormat::read(flags) == 0 ? PixelFormat::BW_1BIT
                                         : PixelFormat::GRAY_2BIT;
#endif
  }
  MAMEFONT_INLINE bool useAltTop() const { return UseAltTop::read(flags); }
  MAMEFONT_INLINE bool useAltBottom() const {
    return UseAltBottom::read(flags);
  }

  Status getBufferShape(uint8_t *numTracks, uint8_t *trackLength) const;
  Status getPixelOffset(int8_t x, int8_t y, frag_index_t *offset,
                               uint8_t *shift = nullptr) const;
  uint8_t getPixel(int8_t x, int8_t y) const;
};

#ifdef MAMEFONT_INCLUDE_IMPL

Status Glyph::getBufferShape(uint8_t *numTracks, uint8_t *trackLength) const {
  bool vertFrag = verticalFragment();
  uint8_t viewport = vertFrag ? glyphHeight : glyphWidth;
  if (pixelFormat() == PixelFormat::BW_1BIT) {
    *numTracks = (viewport + 7) / 8;
  } else {
    *numTracks = (viewport + 3) / 4;
  }
  *trackLength = vertFrag ? glyphWidth : glyphHeight;
  return Status::SUCCESS;
}

Status Glyph::getPixelOffset(int8_t x, int8_t y, frag_index_t *offset,
                             uint8_t *shift) const {
  if (!isValid()) {
    MAMEFONT_THROW_OR_RETURN(Status::GLYPH_NOT_DEFINED);
  }
  if (x < 0 || glyphWidth <= x || y < 0 || glyphHeight <= y) {
    MAMEFONT_THROW_OR_RETURN(Status::COORD_OUT_OF_RANGE);
  }

  uint8_t i, j, stride;
  if (verticalFragment()) {
    i = y;
    j = x;
    stride = glyphWidth;
  } else {
    i = x;
    j = y;
    stride = glyphHeight;
  }

  bool bpp1 = (pixelFormat() == PixelFormat::BW_1BIT);

  if (offset) {
    frag_index_t o = i >> 2;
    if (bpp1) o >>= 1;
    o = o * stride + j;
    *offset = o;
  }

  if (shift) {
    bool farFirst = farPixelFirst();
    if (bpp1) {
      uint8_t s = (i & 7);
      if (farFirst) s = 7 - s;
      *shift = s;
    } else {
      uint8_t s = (i & 3);
      if (farFirst) s = 3 - s;
      *shift = s * 2;
    }
  }

  return Status::SUCCESS;
}

uint8_t Glyph::getPixel(int8_t x, int8_t y) const {
  if (!data) return 0;

  frag_index_t offset;
  uint8_t shift;
  if (Status::SUCCESS != getPixelOffset(x, y, &offset, &shift)) {
    return 0;
  }
  bool bpp1 = (pixelFormat() == PixelFormat::BW_1BIT);
  frag_t frag = data[offset];
  frag >>= shift;
  return bpp1 ? (frag & 0x1) : (frag & 0x3);
}

#endif

}  // namespace mamefont
