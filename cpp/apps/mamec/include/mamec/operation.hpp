#pragma once

#include <memory>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

static constexpr int DUMMY_COST = 99999999;

class OperationClass;
using Operation = std::shared_ptr<OperationClass>;

class OperationClass {
 public:
  const mf::Operator op;
  const std::vector<frag_t> output;
  const int cost;
  const int codeLength;
  const uint8_t code[3];

  OperationClass(mf::Operator op, const std::vector<frag_t> &output,
                 int additionalCost, int byte0 = 0x00, int byte1 = -1,
                 int byte2 = -1)
      : op(op),
        output(output),
        cost(baseCostOf(op) + additionalCost),
        codeLength(byte1 != -1 ? (byte2 != -1 ? 3 : 2) : 1),
        code{(uint8_t)(mf::baseCodeOf(op) | byte0), (uint8_t)byte1,
             (uint8_t)byte2} {
    if (output.empty()) {
      throw std::runtime_error(std::string("Empty output fragments for ") +
                               mf::mnemonicOf(op));
    }
  }

  inline void writeCodeTo(std::vector<uint8_t> &byteCode) const {
    byteCode.push_back(code[0]);
    if (codeLength > 1) {
      byteCode.push_back(code[1]);
      if (codeLength > 2) {
        byteCode.push_back(code[2]);
      }
    }
  }
};

static inline Operation makeOperation(mf::Operator op,
                                      const std::vector<frag_t> &output,
                                      int additionalCost, int byte0 = 0x00,
                                      int byte1 = -1, int byte2 = -1) {
  return std::make_shared<OperationClass>(op, output, additionalCost, byte0,
                                          byte1, byte2);
}

Operation makeLDI(frag_t frag);
Operation makeXOR(int pos, bool width2bit, frag_t frag);
Operation makeLUP(int index, frag_t frag);
Operation makeRPT(frag_t frag, int count);
Operation makeSFT(bool right, bool postSet, int size, int rpt,
                  const std::vector<frag_t> &output);
Operation makeSFI(bool right, bool postSet, bool preShift, int period, int rpt,
                  const std::vector<frag_t> &output);
Operation makeCPY(int offset, int length, bool byteReverse,
                  const std::vector<frag_t> &output);
Operation makeCPX(int offset, int length, uint8_t cpxFlags,
                  const std::vector<frag_t> &output);

}  // namespace mamefont::mamec