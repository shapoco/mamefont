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
  const std::vector<uint8_t> byteCode;
  const std::vector<fragment_t> generated;
  const int cost;

  OperationClass(mf::Operator op, const std::vector<uint8_t> &byteCode,
                 const std::vector<fragment_t> &generated, int cost)
      : op(op), byteCode(byteCode), generated(generated), cost(cost) {
    if (byteCode.empty()) {
      throw std::runtime_error(std::string("Empty bytecode for ") +
                               mf::mnemonicOf(op));
    }
    if (generated.empty()) {
      throw std::runtime_error(std::string("Empty generated fragments for ") +
                               mf::mnemonicOf(op));
    }
  }
};

static inline Operation makeOperation(mf::Operator op,
                            const std::vector<uint8_t> &byteCode,
                            const std::vector<fragment_t> &generated,
                            int cost) {
  return std::make_shared<OperationClass>(op, byteCode, generated, cost);
} 

Operation makeLDI(fragment_t frag);
Operation makeXOR(int pos, bool width2bit, fragment_t frag);
Operation makeLUP(int index, fragment_t frag);
Operation makeRPT(fragment_t frag, int count);
Operation makeSFT(bool right, bool postSet, int size, int rpt,
                  const std::vector<fragment_t> &generated);
Operation makeSFI(bool right, bool postSet, bool preShift, int period, int rpt,
                  const std::vector<fragment_t> &generated);
Operation makeCPY(int offset, int length, bool byteReverse,
                  const std::vector<fragment_t> &generated);
Operation makeCPX(int offset, int length, uint8_t cpxFlags,
                  const std::vector<fragment_t> &generated);

}  // namespace mamefont::mamec