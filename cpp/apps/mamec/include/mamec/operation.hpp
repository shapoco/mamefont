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

static inline Operation makeXOR(int pos, bool width2bit, fragment_t frag) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::XOR);
  byte1 |= mf::XOR_POS::place(pos);
  byte1 |= mf::XOR_WIDTH_2BIT::place(width2bit);
  std::vector<uint8_t> byteCode({byte1});

  std::vector<fragment_t> generated({frag});

  int cost = baseCostOf(mf::Operator::XOR);

  return std::make_shared<OperationClass>(mf::Operator::XOR, byteCode,
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

static inline Operation makeSFI(bool right, bool postSet, bool preShift,
                                int period, int rpt,
                                const std::vector<fragment_t> &generated) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::SFI);
  uint8_t byte2 = 0;
  byte2 |= mf::SFI_PERIOD::place(period);
  byte2 |= mf::SFI_REPEAT_COUNT::place(rpt);
  byte2 |= mf::SFI_RIGHT::place(right);
  byte2 |= mf::SFI_POST_SET::place(postSet);
  byte2 |= mf::SFI_PRE_SHIFT::place(preShift);
  std::vector<uint8_t> byteCode({byte1, byte2});

  int cost = baseCostOf(mf::Operator::SFI);

  return std::make_shared<OperationClass>(mf::Operator::SFI, byteCode,
                                          generated, cost);
}

static inline Operation makeCPY(int offset, int length, bool byteReverse,
                                const std::vector<fragment_t> &generated) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::CPY);
  byte1 |= mf::CPY_OFFSET::place(offset);
  byte1 |= mf::CPY_LENGTH::place(length);
  byte1 |= mf::CPY_BYTE_REVERSE::place(byteReverse);
  std::vector<uint8_t> byteCode({byte1});

  int cost = baseCostOf(mf::Operator::CPY);
  if (byteReverse) cost++;

  return std::make_shared<OperationClass>(mf::Operator::CPY, byteCode,
                                          generated, cost);
}

static inline Operation makeCPX(int offset, int length, uint8_t cpxFlags,
                                const std::vector<fragment_t> &generated) {
  uint8_t byte1 = mf::baseCodeOf(mf::Operator::CPX);

  uint8_t byte2 = offset & 0xFF;

  uint8_t byte3 = 0;
  byte3 |= mf::CPX_OFFSET_H::place(offset >> 8);
  byte3 |= mf::CPX_LENGTH::place(length);
  byte3 |= cpxFlags;  // Include byte reverse, bit reverse, and inverse

  std::vector<uint8_t> byteCode({byte1, byte2, byte3});

  int cost = baseCostOf(mf::Operator::CPX);
  if (mf::CPX_BIT_REVERSE::read(cpxFlags)) cost++;
  if (mf::CPX_INVERSE::read(cpxFlags)) cost++;
  if (mf::CPX_BYTE_REVERSE::read(cpxFlags)) cost++;

  return std::make_shared<OperationClass>(mf::Operator::CPX, byteCode,
                                          generated, cost);
}

}  // namespace mamefont::mamec