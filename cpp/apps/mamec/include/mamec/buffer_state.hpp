#pragma once

#include <memory>
#include <unordered_map>

#include <stdint.h>

#include "mamec/mamec_common.hpp"
#include "mamec/operation.hpp"

namespace mamefont::mamec {
class BufferStateClass;
using BufferState = std::shared_ptr<BufferStateClass>;
class BufferStateClass {
 public:
  const size_t id;
  const int pos;
  const frag_t lastFrag;

  int bestCost = DUMMY_COST;
  Operation bestOpr = nullptr;
  BufferState bestPrev = nullptr;

  BufferState parentState = nullptr;
  std::unordered_map<frag_t, BufferState> childState;

  BufferStateClass(int pos, frag_t lastFrag, BufferState parent)
      : id(nextObjectId()), pos(pos), lastFrag(lastFrag), parentState(parent) {}

  inline void copyPastTo(std::vector<frag_t> &dest, int negativeOffset,
                         int length) const {
    const BufferStateClass *p = this;
    int numSkip = negativeOffset - length;
    for (int i = 0; i < numSkip; i++) {
      if (p) p = p->parentState.get();
    }
    for (int i = 0; i < length; i++) {
      if (p) {
        dest[length - 1 - i] = p->lastFrag;
        p = p->parentState.get();
      } else {
        dest[length - 1 - i] = 0;
      }
    }
  }
};

}  // namespace mamefont::mamec
