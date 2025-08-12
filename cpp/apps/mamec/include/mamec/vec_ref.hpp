#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

struct VecRef {
  const std::vector<frag_t> &fragments;
  const size_t start;
  const size_t size;

  VecRef(const std::vector<frag_t> &frags, size_t start, size_t end)
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

  VecRef(const std::vector<frag_t> &frags)
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

  const inline frag_t &operator[](int i) const {
    return fragments[start + normalizeIndex(i)];
  }

  VecRef slice(int start, int end) const {
    if (start < 0 || end < 0 || start > end) {
      throw std::invalid_argument(
          "Invalid slice range: " + std::to_string(start) + " to " +
          std::to_string(end));
    }
    if (start >= size || end > size) {
      throw std::out_of_range(
          "Slice range out of bounds: " + std::to_string(start) + " to " +
          std::to_string(end));
    }
    return VecRef(fragments, this->start + start, this->start + end);
  }

  inline std::vector<frag_t> toVector(uint8_t cpxFlag = 0) const {
    std::vector<frag_t> result;
    result.reserve(size);
    bool byteReverse = mf::CPX::ByteReverse::read(cpxFlag);
    bool bitReverse = mf::CPX::BitReverse::read(cpxFlag);
    bool inverse = mf::CPX::Inverse::read(cpxFlag);
    for (size_t i = 0; i < size; ++i) {
      int iSrc = byteReverse ? (size - 1 - i) : i;
      frag_t frag = (*this)[iSrc];
      if (bitReverse) frag = mf::reverseBits(frag);
      if (inverse) frag = ~frag;
      result.push_back(frag);
    }
    return std::move(result);
  }
};

static inline bool maskedEqual(const VecRef &a, const VecRef &b,
                               const VecRef &mask, uint8_t cpxFlags = 0) {
  if (a.size != mask.size || b.size != mask.size) {
    throw std::runtime_error(
        "size mismatch in maskedEqual(): a.size=" + std::to_string(a.size) +
        ", b.size=" + std::to_string(b.size) +
        ", mask.size=" + std::to_string(mask.size));
  }
  bool byteReverse = CPX::ByteReverse::read(cpxFlags);
  for (size_t i = 0; i < a.size; ++i) {
    int ia = byteReverse ? (a.size - 1 - i) : i;
    if (!maskedEqual(a[ia], b[i], mask[i], cpxFlags)) return false;
  }
  return true;
}

}  // namespace mamefont::mamec