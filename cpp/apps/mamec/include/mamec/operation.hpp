#pragma once

#include <memory>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

static constexpr int DUMMY_COST = 999999;

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
        cost(cost) {
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

static inline Operation makeLDI(fragment_t frag) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::LDI);
  uint8_t byte2 = frag;
  std::vector<uint8_t> byteCode({byte1, byte2});

  std::vector<fragment_t> generated({frag});

  int cost = baseCostOf(mf::Operator::LDI);

  return std::make_shared<OperationClass>(mf::Operator::LDI, byteCode,
                                          generated, cost);
}

static inline Operation makeLUP(int index, fragment_t frag) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::LUP);
  byte1 |= mf::LUP_INDEX::place(index);
  std::vector<uint8_t> byteCode({byte1});

  std::vector<fragment_t> generated({frag});

  int cost = baseCostOf(mf::Operator::LUP);

  return std::make_shared<OperationClass>(mf::Operator::LUP, byteCode,
                                          generated, cost);
}

static inline Operation makeRPT(fragment_t frag, int count) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::RPT);
  byte1 |= mf::RPT_REPEAT_COUNT::place(count);
  std::vector<uint8_t> byteCode({byte1});

  std::vector<fragment_t> generated;
  generated.resize(count, frag);

  int cost = baseCostOf(mf::Operator::RPT);

  return std::make_shared<OperationClass>(mf::Operator::RPT, byteCode,
                                          generated, cost);
}

static inline Operation makeSFT(bool right, bool postSet, int size, int rpt,
                                const std::vector<fragment_t> &generated) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::SFT);
  byte1 |= mf::SFT_REPEAT_COUNT::place(rpt);
  byte1 |= mf::SFT_SIZE::place(size);
  byte1 |= mf::SFT_POST_SET::place(postSet);
  byte1 |= mf::SFT_RIGHT::place(right);
  std::vector<uint8_t> byteCode({byte1});

  int cost = baseCostOf(mf::Operator::SFT);

  return std::make_shared<OperationClass>(mf::Operator::SFT, byteCode,
                                          generated, cost);
}

}  // namespace mamefont::mamec