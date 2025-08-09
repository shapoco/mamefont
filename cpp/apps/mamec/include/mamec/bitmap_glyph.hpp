#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "mamec/gray_bitmap.hpp"

namespace mamefont::mamec {

class BitmapGlyphClass {
 public:
  int code;
  std::shared_ptr<GrayBitmapClass> bmp;
  int width;
  int height;
  int leftAntiSpace;
  int rightAntiSpace;
  BitmapGlyphClass(int c, int w, int h, std::shared_ptr<GrayBitmapClass> bmp, int las,
              int ras)
      : code(c),
        width(w),
        height(h),
        bmp(bmp),
        leftAntiSpace(las),
        rightAntiSpace(ras) {}
};

using BitmapGlyph = std::shared_ptr<BitmapGlyphClass>;

}  // namespace mamefont::mamec
