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
  static constexpr int DIMENSION_INVALID = -9999;
  std::string fullName;
  std::string familyName;
  int bodySize = DIMENSION_INVALID;
  int capHeight = DIMENSION_INVALID;
  int ascenderSpacing = DIMENSION_INVALID;
  int weight = DIMENSION_INVALID;
  int defaultXSpacing = DIMENSION_INVALID;
  int ySpacing = DIMENSION_INVALID;

  std::vector<std::shared_ptr<BitmapGlyphClass> > glyphs;

  BitmapFontClass(const std::string &path);
  const BitmapGlyph getGlyph(int code) const;

 private:
  std::shared_ptr<BitmapGlyphClass> decodeGlyph(
      int code, const std::shared_ptr<GrayBitmapClass> &bmp, int x, int y);
};

using BitmapFont = std::shared_ptr<BitmapFontClass>;

}  // namespace mamefont::mamec
