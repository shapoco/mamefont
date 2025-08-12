#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "mamec/bitmap_glyph.hpp"

namespace mamefont::mamec {

enum class Dimension {
  BODY_SIZE,
  CAP_HEIGHT,
  ASCENDER_SPACING,
  WEIGHT,
};

std::unordered_map<Dimension, int> parseDimensionIdentifier(
    const std::string &s, size_t startPos);

class BitmapFontClass {
 public:
  std::string fullName;
  std::string familyName;
  int bodySize = -1;
  int capHeight = -1;
  int ascenderSpacing = -1;
  int weight = -1;
  int defaultXSpacing = -1;
  int ySpacing = -1;

  std::vector<std::shared_ptr<BitmapGlyphClass> > glyphs;

  BitmapFontClass(const std::string &path);
  const BitmapGlyph getGlyph(int code) const;

 private:
  std::shared_ptr<BitmapGlyphClass> decodeGlyph(
      int code, const std::shared_ptr<GrayBitmapClass> &bmp, int x, int y);
};

using BitmapFont = std::shared_ptr<BitmapFontClass>;

}  // namespace mamefont::mamec
