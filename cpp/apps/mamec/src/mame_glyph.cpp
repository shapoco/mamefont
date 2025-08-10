#include <iostream>

#include "mamec/mame_glyph.hpp"
#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {
std::vector<frag_t> MameGlyphClass::createCompareMaskArray() const {
  std::vector<frag_t> compareMask;
  int n = fragments.size();
  compareMask.resize(n, 0xFF);
  frag_t mask = lastLaneCompareMask();
  for (int i = n - tall(); i < n; i++) {
    compareMask[i] = mask;
  }
  return std::move(compareMask);
}

void MameGlyphClass::report(std::string indent) const {
  std::vector<uint8_t> byteCode;
  for (const auto &opr : operations) {
    opr->writeCodeTo(byteCode);
  }

  std::cout << indent << "Code: " << formatChar(code) << std::endl;
  std::cout << indent << "Width: " << width << std::endl;
  std::cout << indent << "X Spacing: " << xSpacing << std::endl;
  std::cout << indent << "X Negative Offset: " << xNegativeOffset << std::endl;
  std::cout << indent << "Fragments (" << fragments.size() << " Bytes):" << std::endl;
  dumpByteArray(fragments, indent + "  ");
  std::cout << indent << "Bytecode (" << operations.size() << " ops, " << byteCode.size() << " Bytes):" << std::endl;
  dumpByteArray(byteCode, indent + "  ");
}
}  // namespace mamefont::mamec