#pragma once

#include "mamefont/blob_format.hpp"
#include "mamefont/glyph.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

class Font {
 public:
  const FontHeader header;
  const uint8_t *blob;

  Font(const uint8_t *blob) : header(blob), blob(blob) {}

  MAMEFONT_INLINE uint8_t formatVersion() const { return header.formatVersion; }
  MAMEFONT_INLINE FontFlags flags() const { return header.flags; }
  MAMEFONT_INLINE bool verticalFragment() const {
    return header.flags.verticalFragment();
  }
  MAMEFONT_INLINE bool farPixelFirst() const {
    return header.flags.farPixelFirst();
  }
  MAMEFONT_INLINE PixelFormat fragFormat() const {
    return header.flags.fragFormat();
  }
  MAMEFONT_INLINE bool largeFont() const { return header.flags.largeFont(); }
  MAMEFONT_INLINE bool proportional() const {
    return header.flags.proportional();
  }
  MAMEFONT_INLINE bool hasExtendedHeader() const {
    return header.flags.hasExtendedHeader();
  }
  MAMEFONT_INLINE uint8_t firstCode() const { return header.firstCode; }
  MAMEFONT_INLINE uint8_t lastCode() const { return header.lastCode; }
  MAMEFONT_INLINE uint16_t numGlyphs() const {
    return header.lastCode - header.firstCode + 1;
  }
  MAMEFONT_INLINE uint8_t fragmentTableSize() const {
    return header.fragmentTableSize;
  }
  MAMEFONT_INLINE uint8_t maxGlyphWidth() const { return header.maxGlyphWidth; }
  MAMEFONT_INLINE uint8_t fontHeight() const { return header.fontHeight; }
  MAMEFONT_INLINE uint8_t xSpace() const { return header.xSpace; }
  MAMEFONT_INLINE uint8_t ySpace() const { return header.ySpace; }

  MAMEFONT_INLINE uint16_t getGlyphEntryOffset(uint16_t c) const {
    uint16_t offset = c;
    if (header.flags.largeFont()) offset <<= 1;
    if (header.flags.proportional()) offset <<= 1;
    return FontHeader::SIZE + offset;
  }

  MAMEFONT_INLINE uint16_t fragmentTableOffset() const {
    uint16_t offset =
        getGlyphEntryOffset(header.lastCode - header.firstCode + 1);
    if (offset & 1) offset++;
    return offset;
  }

  MAMEFONT_INLINE uint16_t byteCodeOffset() const {
    return fragmentTableOffset() + header.fragmentTableSize;
  }

  MAMEFONT_INLINE frag_t lookupFragment(int index) const {
    if (index < 0 || header.fragmentTableSize <= index) return 0x00;
    return readBlobU8(blob + fragmentTableOffset() + index);
  }

  frag_index_t calcMaxGlyphBufferSize() const;
  Status getGlyph(uint8_t c, Glyph *glyph) const;
};

#ifdef MAMEFONT_INCLUDE_IMPL

frag_index_t Font::calcMaxGlyphBufferSize() const {
  uint8_t w = header.maxGlyphWidth;
  uint8_t h = header.fontHeight;
  bool bpp1 = (fragFormat() == PixelFormat::BW_1BIT);
  bool verticalFrag = header.flags.verticalFragment();
  uint8_t viewport = verticalFrag ? h : w;
  uint8_t trackLength = verticalFrag ? w : h;
  uint8_t numTracks = bpp1 ? ((viewport + 7) / 8) : ((viewport + 3) / 4);
  return numTracks * trackLength;
}

Status Font::getGlyph(uint8_t c, Glyph *glyph) const {
  if (!glyph) {
    MAMEFONT_THROW_OR_RETURN(Status::NULL_POINTER);
  }

  if (c < header.firstCode || header.lastCode < c) {
    MAMEFONT_THROW_OR_RETURN(Status::CHAR_CODE_OUT_OF_RANGE);
  }

  uint8_t glyphFlags = 0;
  if (header.flags.verticalFragment()) {
    Glyph::VerticalFragment::write(&glyphFlags, true);
  }
  if (header.flags.farPixelFirst()) {
    Glyph::FarPixelFirst::write(&glyphFlags, true);
  }
  Glyph::FragFormat::write(&glyphFlags,
                            static_cast<uint8_t>(header.flags.fragFormat()));

  uint8_t index = c - header.firstCode;
  const uint8_t *ptr = blob + getGlyphEntryOffset(index);

  bool valid;
  if (header.flags.largeFont()) {
    uint16_t val = readBlobU16(ptr);
    ptr += 2;

    valid = (val != 0xFFFF);
    Glyph::Valid::write(&glyphFlags, valid);

    glyph->entryPoint = NormalGlyphEntry::EntryPoint::read(val);

    uint8_t byte2 = val >> 8;
    bool useAltTop = NormalGlyphEntry::UseAltTop::read(byte2);
    if (useAltTop) Glyph::UseAltTop::write(&glyphFlags, true);
    bool useAltBottom = NormalGlyphEntry::UseAltBottom::read(byte2);
    if (useAltBottom) Glyph::UseAltBottom::write(&glyphFlags, true);

    uint8_t top = useAltTop ? header.altTop : 0;
    uint8_t bottom = useAltBottom ? header.altBottom : header.fontHeight;
    glyph->yOffset = top;
    glyph->glyphHeight = bottom - top;
  } else {
    uint8_t val = readBlobU8(ptr++);

    valid = (val != 0xFF);
    Glyph::Valid::write(&glyphFlags, valid);

    glyph->entryPoint = SmallGlyphEntry::EntryPoint::read(val);

    glyph->yOffset = 0;
    glyph->glyphHeight = header.fontHeight;
  }

  glyph->xSpace = header.xSpace;
  if (header.flags.proportional()) {
    if (header.flags.largeFont()) {
      uint8_t byte0 = readBlobU8(ptr++);
      glyph->glyphWidth = NormalGlyphDim::GlyphWidth::read(byte0);
      uint8_t byte1 = readBlobU8(ptr++);
      glyph->xSpace += NormalGlyphDim::XSpaceOffset::read(byte1);
      glyph->xStepBack = NormalGlyphDim::XStepBack::read(byte1);
    } else {
      uint8_t byte0 = readBlobU8(ptr++);
      glyph->glyphWidth = SmallGlyphDim::GlyphWidth::read(byte0);
      glyph->xSpace += SmallGlyphDim::XSpaceOffset::read(byte0);
      glyph->xStepBack = SmallGlyphDim::XStepBack::read(byte0);
    }
  } else {
    glyph->glyphWidth = header.maxGlyphWidth;
    glyph->xStepBack = 0;
  }

  glyph->flags = glyphFlags;
  return valid ? Status::SUCCESS : Status::GLYPH_NOT_DEFINED;
}

#endif

}  // namespace mamefont
