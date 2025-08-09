#pragma once

#include <map>
#include <memory>

#include "mamec/buffer_state.hpp"

namespace mamefont::mamec {

class StateQueue {
 public:
  std::map<int, std::map<int, BufferState>> groups;

  inline bool empty() const { return groups.empty(); }

  inline void put(BufferState state, int newCost) {
    // remove state from the old cost group
    int oldCost = state->bestCost;
    if (groups.contains(oldCost)) {
      auto &sameCostGroup = groups[oldCost];
      if (sameCostGroup.contains(state->id)) {
        if (newCost == oldCost) {
          return;  // no change
        }
        sameCostGroup.erase(state->id);
      }
      if (sameCostGroup.empty()) {
        groups.erase(oldCost);
      }
    }

    // update state cost
    state->bestCost = newCost;

    // add state to the new cost group
    if (groups.contains(newCost)) {
      groups[newCost][state->id] = state;
    } else {
      std::map<int, BufferState> newGroup;
      newGroup[state->id] = state;
      groups[newCost] = std::move(newGroup);
    }
  }

  inline BufferState popBest() {
    auto groupIt = groups.begin();
    auto &bestGroup = groupIt->second;
    auto bestIt = bestGroup.begin();
    BufferState bestState = bestIt->second;
    bestGroup.erase(bestIt);
    if (bestGroup.empty()) {
      groups.erase(groupIt);
    }
    return bestState;
  }
};

}  // namespace mamefont::mamec