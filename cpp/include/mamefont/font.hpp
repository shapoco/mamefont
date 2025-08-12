#pragma once

#include "mamefont/mamefont_common.hpp"

namespace mamefont {

class Font {
 public:
  const FontHeader header;
  const uint8_t *blob;

  Font(const uint8_t *blob) : header(blob), blob(blob) {}

  MAMEFONT_NOINLINE frag_index_t calcGlyphBufferSize(const Glyph *glyph,
                                                     uint8_t *stride) const {
    uint8_t w = (header.flags.proportional() && glyph) ? glyph->glyphWidth
                                                       : header.maxGlyphWidth;
    uint8_t h = header.glyphHeight;
    bool verticalFrag = header.flags.verticalFragment();
    uint8_t viewport = verticalFrag ? h : w;
    uint8_t trackLength = verticalFrag ? w : h;
    uint8_t numTracks = (uint8_t)(viewport + 7) / 8;
    if (stride) {
      *stride = verticalFrag ? trackLength : numTracks;
    }
    return numTracks * trackLength;
  }

  MAMEFONT_INLINE uint8_t formatVersion() const { return header.formatVersion; }
  MAMEFONT_INLINE FontFlags flags() const { return header.flags; }
  MAMEFONT_INLINE bool verticalFragment() const {
    return header.flags.verticalFragment();
  }
  MAMEFONT_INLINE bool msb1st() const { return header.flags.msb1st(); }
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
  MAMEFONT_INLINE uint8_t glyphHeight() const { return header.glyphHeight; }
  MAMEFONT_INLINE uint8_t xMonoSpacing() const { return header.xMonoSpacing; }
  MAMEFONT_INLINE uint8_t ySpacing() const { return header.ySpacing; }

  MAMEFONT_INLINE frag_index_t calcGlyphBufferSize(uint8_t *stride) const {
    return calcGlyphBufferSize(nullptr, stride);
  }

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

  MAMEFONT_NOINLINE Status getGlyph(uint8_t c, Glyph *glyph) const {
    if (!glyph) {
      MAMEFONT_RETURN_ERROR(Status::NULL_POINTER);
    }

    if (c < header.firstCode || header.lastCode < c) {
      MAMEFONT_RETURN_ERROR(Status::CHAR_CODE_OUT_OF_RANGE);
    }

    uint8_t index = c - header.firstCode;
    const uint8_t *ptr = blob + getGlyphEntryOffset(index);

    bool valid;
    uint16_t entryPoint;
    if (header.flags.largeFont()) {
      uint16_t val = readBlobU16(ptr);
      ptr += 2;
      valid = val != 0xFFFF;
      entryPoint = NormalGlyphEntry::EntryPoint::read(val);
    } else {
      uint8_t val = readBlobU8(ptr++);
      valid = val != 0xFF;
      entryPoint = SmallGlyphEntry::EntryPoint::read(val);
    }
    glyph->entryPoint = valid ? entryPoint : DUMMY_ENTRY_POINT;

    if (header.flags.proportional()) {
      if (header.flags.largeFont()) {
        uint8_t byte0 = readBlobU8(ptr++);
        glyph->glyphWidth = NormalGlyphDimension::GlyphWidth::read(byte0);
        uint8_t byte1 = readBlobU8(ptr++);
        glyph->xSpacing = NormalGlyphDimension::XSpacing::read(byte1);
        glyph->xStepBack = NormalGlyphDimension::XStepBack::read(byte1);
      } else {
        uint8_t byte0 = readBlobU8(ptr++);
        glyph->glyphWidth = SmallGlyphDimension::GlyphWidth::read(byte0);
        glyph->xSpacing = SmallGlyphDimension::XSpacing::read(byte0);
        glyph->xStepBack = SmallGlyphDimension::XStepBack::read(byte0);
      }
    } else {
      glyph->glyphWidth = header.maxGlyphWidth;
      glyph->xSpacing = header.xMonoSpacing;
      glyph->xStepBack = 0;
    }

    return valid ? Status::SUCCESS : Status::GLYPH_NOT_DEFINED;
  }

#ifdef MAMEFONT_DEBUG
  frag_t lookupFragment(int index) const {
    if (index < 0 || header.fragmentTableSize <= index) return 0x00;
    return readBlobU8(blob + fragmentTableOffset() + index);
  }
#endif
};

}  // namespace mamefont
