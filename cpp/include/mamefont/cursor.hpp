#pragma once

#include "mamefont/mamefont_common.hpp"

namespace mamefont {

struct AddrRule {
  int8_t lanesPerGlyph;  // Number of lanes per glyph
  int8_t fragsPerLane;   // Number of fragments per lane
#ifdef MAMEFONT_HORIZONTAL_FRAGMENT_ONLY
  static constexpr uint8_t laneStride = 1;
#else
  frag_index_t laneStride;  // Stride to next lane in bytes
#endif
#ifdef MAMEFONT_VERTICAL_FRAGMENT_ONLY
  static constexpr uint8_t fragStride = 1;
#else
  frag_index_t fragStride;  // Stride to next fragment in bytes
#endif
};

struct Cursor {
  frag_index_t offset;      // Offset from start of glyph in bytes
  frag_index_t laneOffset;  // Offset of first fragment of current lane
  int8_t fragIndex;         // Distance from start of current lane in bytes

  void reset() {
    offset = 0;
    laneOffset = 0;
    fragIndex = 0;
  }

  MAMEFONT_INLINE void add(const AddrRule &rule, frag_index_t delta) {
    if (delta >= 0) {
      while (delta >= rule.fragsPerLane) {
        delta -= rule.fragsPerLane;
        laneOffset += rule.laneStride;
        offset += rule.laneStride;
      }
      fragIndex += delta;
      offset += delta * rule.fragStride;
      if (fragIndex >= rule.fragsPerLane) {
        fragIndex -= rule.fragsPerLane;
        laneOffset += rule.laneStride;
        offset = laneOffset + fragIndex * rule.fragStride;
      }
    } else {
      delta = -delta;
      while (delta >= rule.fragsPerLane) {
        delta -= rule.fragsPerLane;
        laneOffset -= rule.laneStride;
        offset -= rule.laneStride;
      }
      fragIndex -= delta;
      offset -= delta * rule.fragStride;
      if (fragIndex < 0) {
        laneOffset -= rule.laneStride;
        fragIndex += rule.fragsPerLane;
        offset = laneOffset + fragIndex * rule.fragStride;
      }
    }
  }

  MAMEFONT_INLINE frag_index_t postIncr(const AddrRule &rule) {
    frag_index_t oldOffset = offset;
    offset += rule.fragStride;
    fragIndex++;
    if (fragIndex >= rule.fragsPerLane) {
      fragIndex = 0;
      laneOffset += rule.laneStride;
      offset = laneOffset;
    }
    return oldOffset;
  }

  MAMEFONT_INLINE frag_index_t preDecr(const AddrRule &rule) {
    offset -= rule.fragStride;
    fragIndex--;
    if (fragIndex < 0) {
      fragIndex = rule.fragsPerLane - 1;
      laneOffset -= rule.laneStride;
      offset = laneOffset + fragIndex * rule.fragStride;
    }
    return offset;
  }
};

}
