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
  std::cout << indent << "Code: " << formatChar(code) << std::endl;
  std::cout << indent << "Width: " << width << std::endl;
  std::cout << indent << "X Spacing: " << xSpacing << std::endl;
  std::cout << indent << "X Negative Offset: " << xNegativeOffset
            << std::endl;
  std::cout << indent << "Fragments:" << std::endl;
  for (int i = 0; i < fragments.size(); ++i) {
    if (i % 16 == 0) std::cout << indent << "  ";
    std::cout << byteToHexStr(fragments[i]) << " ";
    if ((i + 1) % 16 == 0 || i == fragments.size() - 1) {
      std::cout << std::endl;
    }
  }
  std::vector<uint8_t> byteCode;
  for (const auto &opr : operations) {
    opr->writeCodeTo(byteCode);
  }
  std::cout << indent << "Bytecode:" << std::endl;
  for (int i = 0; i < byteCode.size(); ++i) {
    if (i % 16 == 0) std::cout << indent << "  ";
    std::cout << byteToHexStr(byteCode[i]) << " ";
    if ((i + 1) % 16 == 0 || i == byteCode.size() - 1) {
      std::cout << std::endl;
    }
  }
}
}  // namespace mamefont::mamec