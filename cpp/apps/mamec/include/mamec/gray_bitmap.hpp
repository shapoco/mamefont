#pragma once

#include <memory>
#include <string>
#include <vector>

#include <stdint.h>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

class GrayBitmapClass;

using GrayBitmap = std::shared_ptr<GrayBitmapClass>;

class GrayBitmapClass {
 public:
  static constexpr int16_t MARKER_GLYPH = -1;
  static constexpr int16_t MARKER_SPACING = -2;

  std::vector<int16_t> data;
  int width;
  int height;

  GrayBitmapClass(std::vector<int16_t>& data, int width, int height);
  GrayBitmapClass(const std::string& path);
  int16_t get(int x, int y, int16_t defaultColor = -1) const;

  std::shared_ptr<GrayBitmapClass> crop(int x, int y, int w, int h) const;

  std::vector<frag_t> toFragments(bool verticalFrag = false,
                                        bool msb1st = false) const;
};

}  // namespace mamefont::mamec