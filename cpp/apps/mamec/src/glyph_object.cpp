#include <iostream>

#include "mamec/glyph_object.hpp"
#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

void GlyphObjectClass::replaceOperation(int index, const Operation &newOpr) {
  if (index < 0 || index >= operations.size()) {
    throw std::out_of_range("Index " + std::to_string(index) +
                            " is out of range in replaceOperation()");
  }
  newOpr->beforeBarrier = operations[index]->beforeBarrier;
  newOpr->afterBarrier = operations[index]->afterBarrier;
  operations[index] = newOpr;
}

void GlyphObjectClass::report(std::string indent) const {
  std::vector<uint8_t> byteCode;
  for (const auto &opr : operations) {
    opr->writeCodeTo(byteCode);
  }

  std::cout << indent << "Code: " << c2s(code) << std::endl;
  std::cout << indent << "Width: " << width << std::endl;
  std::cout << indent << "X Spacing: " << xSpacing << std::endl;
  std::cout << indent << "X Negative Offset: " << xStepBack << std::endl;
  std::cout << indent << "Fragments (" << fragments.size()
            << " Bytes):" << std::endl;
  dumpByteArray(fragments, indent + "  ");
  std::cout << indent << "Bytecode (" << operations.size() << " ops, "
            << byteCode.size() << " Bytes):" << std::endl;
  dumpByteArray(byteCode, indent + "  ");
}
}  // namespace mamefont::mamec
