#include <memory>
#include <vector>

#include "mamec/operation.hpp"

namespace mamefont::mamec {

Operation makeLDI(frag_t frag, int addCost) {
  std::vector<frag_t> output({frag});
  return makeOperation(mf::Operator::LDI, output, addCost, 0x00, frag);
}

Operation makeXOR(int pos, bool width2bit, frag_t frag) {
  uint8_t arg1 = 0;
  arg1 |= mf::XOR::Pos::place(pos);
  arg1 |= mf::XOR::Width2Bit::place(width2bit);
  std::vector<frag_t> output({frag});
  return makeOperation(mf::Operator::XOR, output, 0, arg1);
}

Operation makeLUP(int index, frag_t frag) {
  uint8_t arg1 = mf::LUP::Index::place(index);
  std::vector<frag_t> output({frag});
  return makeOperation(mf::Operator::LUP, output, 0, arg1);
}

Operation makeLUD(int index, int step, frag_t frag1, frag_t frag2) {
  uint8_t arg1 = 0;
  arg1 |= mf::LUD::Index::place(index);
  arg1 |= mf::LUD::Step::place(step == 1);
  std::vector<frag_t> output({frag1, frag2});
  return makeOperation(mf::Operator::LUD, output, 0, arg1);
}

Operation makeRPT(frag_t frag, int count) {
  uint8_t arg1 = mf::RPT::RepeatCount::place(count);
  std::vector<frag_t> output;
  output.resize(count, frag);
  return makeOperation(mf::Operator::RPT, output, 0, arg1);
}

Operation makeSFT(bool right, bool postSet, int size, int rpt,
                  const std::vector<frag_t> &output) {
  uint8_t arg1 = 0;
  arg1 |= mf::SFT::RepeatCount::place(rpt);
  arg1 |= mf::SFT::Size::place(size);
  arg1 |= mf::SFT::PostSet::place(postSet);
  arg1 |= mf::SFT::Right::place(right);
  return makeOperation(mf::Operator::SFT, output, 0, arg1);
}

Operation makeSFI(bool right, bool postSet, bool preShift, int period, int rpt,
                  const std::vector<frag_t> &output) {
  uint8_t arg2 = 0;
  arg2 |= mf::SFI::Period::place(period);
  arg2 |= mf::SFI::RepeatCount::place(rpt);
  arg2 |= mf::SFI::Right::place(right);
  arg2 |= mf::SFI::PostSet::place(postSet);
  arg2 |= mf::SFI::PreShift::place(preShift);
  return makeOperation(mf::Operator::SFI, output, 0, 0x00, arg2);
}

Operation makeCPY(int offset, int length, bool byteReverse,
                  const std::vector<frag_t> &output) {
  uint8_t arg1 = 0;
  arg1 |= mf::CPY::Offset::place(offset);
  arg1 |= mf::CPY::Length::place(length);
  arg1 |= mf::CPY::ByteReverse::place(byteReverse);

  int addCost = byteReverse ? 1 : 0;

  return makeOperation(mf::Operator::CPY, output, addCost, arg1);
}

Operation makeCPX(int offset, int length, uint8_t cpxFlags,
                  const std::vector<frag_t> &output) {
  uint8_t arg23[2] = {0x00};
  mf::CPX::Offset::write(arg23, offset);
  uint8_t &arg2 = arg23[0];
  uint8_t &arg3 = arg23[1];
  arg3 |= mf::CPX::Length::place(length);
  arg3 |= cpxFlags;  // Include byte reverse, bit reverse, and inverse

  int addCost = 0;
  if (mf::CPX::BitReverse::read(cpxFlags)) addCost++;
  if (mf::CPX::Inverse::read(cpxFlags)) addCost++;
  if (mf::CPX::ByteReverse::read(cpxFlags)) addCost++;

  return makeOperation(mf::Operator::CPX, output, addCost, 0x00, arg2, arg3);
}
}  // namespace mamefont::mamec
