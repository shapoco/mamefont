#pragma once

#include <memory>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

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
      : op(op),
        byteCode(std::move(byteCode)),
        generated(std::move(generated)),
        cost(cost) {}
};

static Operation makeLDI(fragment_t frag) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::LDI);
  uint8_t byte2 = frag;
  std::vector<uint8_t> byteCode({byte1, byte2});

  std::vector<fragment_t> generated({frag});

  int cost = baseCostOf(mf::Operator::LDI);

  return std::make_shared<OperationClass>(mf::Operator::LDI, byteCode,
                                          generated, cost);
}

static Operation makeLUP(int index, fragment_t frag) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::LUP);
  byte1 |= mf::LUP_INDEX::place(index);
  std::vector<uint8_t> byteCode({byte1});

  std::vector<fragment_t> generated({frag});

  int cost = baseCostOf(mf::Operator::LUP);

  return std::make_shared<OperationClass>(mf::Operator::LUP, byteCode,
                                          generated, cost);
}

static Operation makeRPT(fragment_t frag, int count) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::RPT);
  byte1 |= mf::RPT_REPEAT_COUNT::place(count);
  std::vector<uint8_t> byteCode({byte1});

  std::vector<fragment_t> generated;
  generated.resize(count, frag);

  int cost = baseCostOf(mf::Operator::RPT);

  return std::make_shared<OperationClass>(mf::Operator::RPT, byteCode,
                                          generated, cost);
}

}  // namespace mamefont::mamec