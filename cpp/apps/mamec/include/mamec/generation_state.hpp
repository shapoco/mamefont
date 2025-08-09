#pragma once

#include <memory>
#include <unordered_map>

#include <stdint.h>

#include "mamec/mamec_common.hpp"
#include "mamec/operation.hpp"

namespace mamefont::mamec {
class GenerationStateClass;
using GenerationState = std::shared_ptr<GenerationStateClass>;
class GenerationStateClass {
 public:
  const size_t id;
  const int pos;
  const fragment_t lastFrag;
  int bestCost = 999999;

  Operation bestOpr = nullptr;
  GenerationState bestPrev = nullptr;

  GenerationState lastState = nullptr;
  std::unordered_map<fragment_t, GenerationState> nextState;

  GenerationStateClass(int pos, fragment_t lastFrag, GenerationState lastState)
      : id(nextObjectId()),
        pos(pos),
        lastFrag(lastFrag),
        lastState(lastState) {}
};

}  // namespace mamefont::mamec
