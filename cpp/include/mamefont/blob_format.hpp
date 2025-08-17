#pragma once

#include "mamefont/bit_field.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

struct FontFlags {
  using VerticalFragment = BitFlag<0, 7>;
  using FarPixelFirst = BitFlag<0, 6>;
  using FragFormat = BitField<uint8_t, uint8_t, 0, 4, 2>;
  using LargeFont = BitFlag<0, 2>;
  using Proportional = BitFlag<0, 1>;
  using HasExtendedHeader = BitFlag<0, 0>;

  uint8_t value;
  FontFlags() : value(0) {}
  FontFlags(uint8_t flags) : value(flags) {}

  MAMEFONT_INLINE bool verticalFragment() const {
#ifdef MAMEFONT_HORI_FRAG_ONLY
    return false;
#elif defined(MAMEFONT_VERT_FRAG_ONLY)
    return true;
#else
    return VerticalFragment::read(value);
#endif
  }

  MAMEFONT_INLINE bool farPixelFirst() const {
#ifdef MAMEFONT_NEAR1ST_ONLY
    return false;
#elif defined(MAMEFONT_FAR1ST_ONLY)
    return true;
#else
    return FarPixelFirst::read(value);
#endif
  }

  MAMEFONT_INLINE PixelFormat fragFormat() const {
#if defined(MAMEFONT_1BPP_ONLY)
    return PixelFormat::BW_1BIT;
#elif defined(MAMEFONT_2BPP_ONLY)
    return PixelFormat::GRAY_2BIT;
#else
    return FragFormat::read(value) == 0 ? PixelFormat::BW_1BIT
                                        : PixelFormat::GRAY_2BIT;
#endif
  }

  MAMEFONT_INLINE bool largeFont() const {
#ifdef MAMEFONT_SMALL_ONLY
    return false;
#elif defined(MAMEFONT_LARGE_ONLY)
    return true;
#else
    return LargeFont::read(value);
#endif
  }

  MAMEFONT_INLINE bool proportional() const {
#ifdef MAMEFONT_MONOSPACED_ONLY
    return false;
#elif defined(MAMEFONT_PROPORTIONAL_ONLY)
    return true;
#else
    return Proportional::read(value);
#endif
  }

  MAMEFONT_INLINE bool hasExtendedHeader() const {
#ifdef MAMEFONT_NO_EXT_HEADER
    return false;
#else
    return HasExtendedHeader::read(value);
#endif
  }
};
struct FontHeader {
  static constexpr uint8_t SIZE = 12;
  using FormatVersion = BitField<uint8_t, uint8_t, 0, 0, 8>;
  using Flags = BitField<uint8_t, uint8_t, 1, 0, 8>;
  using FirstCode = BitField<uint8_t, uint8_t, 2, 0, 8>;
  using LastCode = BitField<uint8_t, uint8_t, 3, 0, 8>;
  using MaxGlyphWidth = BitField<uint8_t, uint8_t, 4, 0, 6, 1>;
  using GlyphHeight = BitField<uint8_t, uint8_t, 5, 0, 6, 1>;
  using XSpace = BitField<int8_t, uint8_t, 6, 0, 6, -32>;
  using YSpace = BitField<uint8_t, uint8_t, 7, 0, 6>;
  using YStepBack = BitField<uint8_t, uint8_t, 8, 0, 6>;
  using AltTop = BitField<uint8_t, uint8_t, 9, 0, 6>;
  using AltBottom = BitField<uint8_t, uint8_t, 10, 0, 6, 1>;
  using FragmentTableSize = BitField<uint8_t, uint8_t, 11, 0, 5, 2, 2>;

  uint8_t formatVersion;
  FontFlags flags;
  uint8_t firstCode;
  uint8_t lastCode;
  uint8_t maxGlyphWidth;
  uint8_t fontHeight;
  int8_t xSpace;
  uint8_t ySpace;
  uint8_t yStepBack;
  uint8_t altTop;
  uint8_t altBottom;
  uint8_t fragmentTableSize;

  FontHeader() = default;
  FontHeader(const uint8_t *blob) { loadFromBlob(blob); }

  MAMEFONT_NOINLINE void loadFromBlob(const uint8_t *blob) {
    const uint8_t *ptr = blob;
    uint8_t byte;
    formatVersion = FontHeader::FormatVersion::read(readBlobU8(ptr++));
    flags = FontHeader::Flags::read(readBlobU8(ptr++));
    firstCode = FontHeader::FirstCode::read(readBlobU8(ptr++));
    lastCode = FontHeader::LastCode::read(readBlobU8(ptr++));
    maxGlyphWidth = FontHeader::MaxGlyphWidth::read(readBlobU8(ptr++));
    fontHeight = FontHeader::GlyphHeight::read(readBlobU8(ptr++));
    xSpace = FontHeader::XSpace::read(readBlobU8(ptr++));
    ySpace = FontHeader::YSpace::read(readBlobU8(ptr++));
    yStepBack = FontHeader::YStepBack::read(readBlobU8(ptr++));
    altTop = FontHeader::AltTop::read(readBlobU8(ptr++));
    altBottom = FontHeader::AltBottom::read(readBlobU8(ptr++));
    fragmentTableSize = FontHeader::FragmentTableSize::read(readBlobU8(ptr++));
  }

#ifdef MAMEFONT_DEBUG
  void dumpHeader(const char *indent) const {
    printf("%sFormat Version  : %d\n", indent, formatVersion);
    printf("%sFlags           : 0x%02X\n", indent, flags.value);
    printf("%s  Vertical Frag.  : %s\n", indent,
           flags.verticalFragment() ? "Yes" : "No");
    printf("%s  Far Pixel First : %s\n", indent,
           flags.farPixelFirst() ? "Yes" : "No");
    printf("%s  Bits per Pixel  : %s\n", indent,
           flags.fragFormat() == PixelFormat::BW_1BIT ? "1" : "2");
    printf("%s  Large Font      : %s\n", indent,
           flags.largeFont() ? "Yes" : "No");
    printf("%s  Proportional    : %s\n", indent,
           flags.proportional() ? "Yes" : "No");
    printf("%s  Has Ext. Header : %s\n", indent,
           flags.hasExtendedHeader() ? "Yes" : "No");
    printf("%sFirst Code      : 0x%02X\n", indent, firstCode);
    printf("%sLast Code       : 0x%02X\n", indent, lastCode);
    printf("%sMax Glyph Width : %d\n", indent, maxGlyphWidth);
    printf("%sGlyph Height    : %d\n", indent, fontHeight);
    printf("%sX Spacing Base  : %d\n", indent, xSpace);
    printf("%sY Spacing       : %d\n", indent, ySpace);
    printf("%sY Step Back     : %d\n", indent, yStepBack);
    printf("%sAlt Top         : %d\n", indent, altTop);
    printf("%sAlt Bottom      : %d\n", indent, altBottom);
    printf("%sFrag Table Size : %d\n", indent, fragmentTableSize);
  }
#endif
};

struct ExtendedHeader {
  using Size = BitField<uint16_t, uint8_t, 0, 0, 8, 2, 2>;
};

struct NormalGlyphEntry {
  static constexpr uint8_t SIZE = 2;
  using UseAltTop = BitFlag<1, 6>;
  using UseAltBottom = BitFlag<1, 7>;
  using EntryPoint = BitField<uint16_t, uint16_t, 0, 0, 14>;
};

struct SmallGlyphEntry {
  static constexpr uint8_t SIZE = 1;
  using EntryPoint = BitField<uint16_t, uint8_t, 0, 0, 8, 0, 2>;
};

struct NormalGlyphDim {
  static constexpr uint8_t SIZE = 2;
  using GlyphWidth = BitField<uint8_t, uint8_t, 0, 0, 6, 1>;
  using XSpaceOffset = BitField<int8_t, int8_t, 1, 0, 4>;
  using XStepBack = BitField<uint8_t, uint8_t, 1, 4, 4>;
};

struct SmallGlyphDim {
  static constexpr uint8_t SIZE = 1;
  using GlyphWidth = BitField<uint8_t, uint8_t, 0, 0, 4, 1>;
  using XSpaceOffset = BitField<int8_t, int8_t, 0, 4, 2>;
  using XStepBack = BitField<uint8_t, uint8_t, 0, 6, 2>;
};

}  // namespace mamefont