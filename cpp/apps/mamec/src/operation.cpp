#include <memory>
#include <vector>

#include "mamec/operation.hpp"

namespace mamefont::mamec {

Operation makeLDI(frag_t frag) {
  std::vector<frag_t> output({frag});
  return makeOperation(mf::Operator::LDI, output, 0, 0x00, frag);
}

Operation makeXOR(int pos, bool width2bit, frag_t frag) {
  uint8_t arg1 = 0;
  arg1 |= mf::XOR_POS::place(pos);
  arg1 |= mf::XOR_WIDTH_2BIT::place(width2bit);
  std::vector<frag_t> output({frag});
  return makeOperation(mf::Operator::XOR, output, 0, arg1);
}

Operation makeLUP(int index, frag_t frag) {
  uint8_t arg1 = mf::LUP_INDEX::place(index);
  std::vector<frag_t> output({frag});
  return makeOperation(mf::Operator::LUP, output, 0, arg1);
}

Operation makeLUD(int index, int step, frag_t frag1, frag_t frag2) {
  uint8_t arg1 = 0;
  arg1 |= mf::LUD_INDEX::place(index);
  arg1 |= mf::LUD_STEP::place(step == 1);
  std::vector<frag_t> output({frag1, frag2});
  return makeOperation(mf::Operator::LUD, output, 0, arg1);
}

Operation makeRPT(frag_t frag, int count) {
  uint8_t arg1 = mf::RPT_REPEAT_COUNT::place(count);
  std::vector<frag_t> output;
  output.resize(count, frag);
  return makeOperation(mf::Operator::RPT, output, 0, arg1);
}

Operation makeSFT(bool right, bool postSet, int size, int rpt,
                  const std::vector<frag_t> &output) {
  uint8_t arg1 = 0;
  arg1 |= mf::SFT_REPEAT_COUNT::place(rpt);
  arg1 |= mf::SFT_SIZE::place(size);
  arg1 |= mf::SFT_POST_SET::place(postSet);
  arg1 |= mf::SFT_RIGHT::place(right);
  return makeOperation(mf::Operator::SFT, output, 0, arg1);
}

Operation makeSFI(bool right, bool postSet, bool preShift, int period, int rpt,
                  const std::vector<frag_t> &output) {
  uint8_t arg2 = 0;
  arg2 |= mf::SFI_PERIOD::place(period);
  arg2 |= mf::SFI_REPEAT_COUNT::place(rpt);
  arg2 |= mf::SFI_RIGHT::place(right);
  arg2 |= mf::SFI_POST_SET::place(postSet);
  arg2 |= mf::SFI_PRE_SHIFT::place(preShift);
  return makeOperation(mf::Operator::SFI, output, 0, 0x00, arg2);
}

Operation makeCPY(int offset, int length, bool byteReverse,
                  const std::vector<frag_t> &output) {
  uint8_t arg1 = 0;
  arg1 |= mf::CPY_OFFSET::place(offset);
  arg1 |= mf::CPY_LENGTH::place(length);
  arg1 |= mf::CPY_BYTE_REVERSE::place(byteReverse);

  int addCost = byteReverse ? 1 : 0;

  return makeOperation(mf::Operator::CPY, output, addCost, arg1);
}

Operation makeCPX(int offset, int length, uint8_t cpxFlags,
                  const std::vector<frag_t> &output) {
  uint8_t arg2 = offset & 0xFF;
  uint8_t arg3 = 0;
  arg3 |= mf::CPX_OFFSET_H::place(offset >> 8);
  arg3 |= mf::CPX_LENGTH::place(length);
  arg3 |= cpxFlags;  // Include byte reverse, bit reverse, and inverse

  int addCost = 0;
  if (mf::CPX_BIT_REVERSE::read(cpxFlags)) addCost++;
  if (mf::CPX_INVERSE::read(cpxFlags)) addCost++;
  if (mf::CPX_BYTE_REVERSE::read(cpxFlags)) addCost++;

  return makeOperation(mf::Operator::CPX, output, addCost, 0x00, arg2, arg3);
}
}  // namespace mamefont::mamec
