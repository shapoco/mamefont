#pragma once

#include "mamec/gray_bitmap.hpp"
#include "mamec/mamec_common.hpp"
#include "mamec/operation.hpp"

namespace mamefont::mamec {

class MameGlyphClass;

using MameGlyph = std::shared_ptr<MameGlyphClass>;

class MameGlyphClass {
 public:
  int code;
  std::vector<fragment_t> fragments;
  bool verticalFragment = false;
  bool msb1st = false;
  int width = 0;
  int height = 0;
  int xSpacing = 0;
  int xNegativeOffset = 0;

  std::vector<Operation> operations;
  int fragmentsSameAsCode = -1;
  uint16_t entryPoint = mf::ENTRYPOINT_DUMMY;
  int byteCodeSize = 0;

  MameGlyphClass(int code, std::vector<fragment_t> frags, int width, int height,
                 bool vertFrag, bool msb1st, int xSpacing, int xNegOffset)
      : code(code),
        fragments(frags),
        width(width),
        height(height),
        verticalFragment(vertFrag),
        msb1st(msb1st),
        xSpacing(xSpacing),
        xNegativeOffset(xNegOffset) {}

  inline int numLanes() const {
    return ((verticalFragment ? height : width) + 7) / 8;
  }

  inline int tall() const { return verticalFragment ? width : height; }
  inline int thickness() const { return verticalFragment ? height : width; }
  inline int lastLaneThickness() const {
    return thickness() - (numLanes() - 1) * 8;
  }

  inline fragment_t lastLaneCompareMask() const {
    int w = lastLaneThickness();
    fragment_t mask = (1 << w) - 1;
    if (msb1st) {
      mask <<= (8 - w);
    }
    return mask;
  }

  std::vector<fragment_t> createCompareMaskArray() const;
  void report(std::string indent) const;
};
}  // namespace mamefont::mamec
