#pragma once

#include "mamec/gray_bitmap.hpp"
#include "mamec/mamec_common.hpp"
#include "mamec/operation.hpp"

namespace mamefont::mamec {

class GlyphObjectClass;

using GlyphObject = std::shared_ptr<GlyphObjectClass>;

class GlyphObjectClass {
 public:
  int code;
  std::vector<frag_t> fragments;
  bool verticalFragment = false;
  bool msb1st = false;
  int width = 0;
  int height = 0;
  int xSpacing = 0;
  int xStepBack = 0;

  std::vector<Operation> operations;
  int fragmentsSameAsCode = -1;
  uint16_t entryPoint = mf::DUMMY_ENTRY_POINT;
  int byteCodeSize = 0;

  GlyphObjectClass(int code, std::vector<frag_t> frags, int width, int height,
                 bool vertFrag, bool msb1st, int xSpacing, int xNegOffset)
      : code(code),
        fragments(frags),
        width(width),
        height(height),
        verticalFragment(vertFrag),
        msb1st(msb1st),
        xSpacing(xSpacing),
        xStepBack(xNegOffset) {}

  inline int numLanes() const {
    return ((verticalFragment ? height : width) + 7) / 8;
  }

  inline int tall() const { return verticalFragment ? width : height; }
  inline int thickness() const { return verticalFragment ? height : width; }
  inline int lastLaneThickness() const {
    return thickness() - (numLanes() - 1) * 8;
  }

  inline frag_t lastLaneCompareMask() const {
    int w = lastLaneThickness();
    frag_t mask = (1 << w) - 1;
    if (msb1st) {
      mask <<= (8 - w);
    }
    return mask;
  }

  std::vector<frag_t> createCompareMaskArray() const;
  void report(std::string indent) const;
};
}  // namespace mamefont::mamec
