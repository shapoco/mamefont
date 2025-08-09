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
  const fragment_t lastFrag;

  int bestCost = DUMMY_COST;
  Operation bestOpr = nullptr;
  BufferState bestPrev = nullptr;

  BufferState parentState = nullptr;
  std::unordered_map<fragment_t, BufferState> childState;

  BufferStateClass(int pos, fragment_t lastFrag, BufferState parent)
      : id(nextObjectId()), pos(pos), lastFrag(lastFrag), parentState(parent) {}
};

}  // namespace mamefont::mamec
