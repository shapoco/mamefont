#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

struct VecRef {
  const std::vector<fragment_t> &fragments;
  const size_t start;
  const size_t size;

  VecRef(const std::vector<fragment_t> &frags, size_t start, size_t end)
      : fragments(frags), start(start), size(end - start) {
    if (start < 0 || start >= frags.size()) {
      throw std::out_of_range("Start index " + std::to_string(start) +
                              " is out of range in VecRef()");
    }
    if (end < start || end > frags.size()) {
      throw std::out_of_range("End index " + std::to_string(end) +
                              " is out of range in VecRef()");
    }
    if (start > end) {
      throw std::invalid_argument("Start index " + std::to_string(start) +
                                  " cannot be greater than end index " +
                                  std::to_string(end) + " in VecRef()");
    }
  }

  VecRef(const std::vector<fragment_t> &frags)
      : fragments(frags), start(0), size(frags.size()) {}

  inline int normalizeIndex(int i) const {
    if (i < 0) i = size + i;
    if (i < 0 || size <= i) {
      throw std::out_of_range("Index " + std::to_string(i) +
                              " out of range 0.." + std::to_string(size - 1) +
                              " in normalizeIndex()");
    }
    return i;
  }

  const inline fragment_t &operator[](int i) const {
    return fragments[start + normalizeIndex(i)];
  }

  VecRef slice(int start, int end) const {
    if (start < 0 || end < 0 || start > end) {
      throw std::invalid_argument("Invalid slice range: " +
                                  std::to_string(start) + " to " +
                                  std::to_string(end));
    }
    if (start >= size || end > size) {
      throw std::out_of_range("Slice range out of bounds: " +
                              std::to_string(start) + " to " +
                              std::to_string(end));
    }
    return VecRef(fragments, this->start + start, this->start + end);
  }
};

}  // namespace mamefont::mamec