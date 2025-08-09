#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "mamec/bitmap_glyph.hpp"
#include "mamec/buffer_state.hpp"
#include "mamec/encoder.hpp"
#include "mamec/gray_bitmap.hpp"
#include "mamec/mame_glyph.hpp"
#include "mamec/state_queue.hpp"
#include "mamec/vec_ref.hpp"

#define FOR_FIELD_VALUES(field, value) \
  for (int value = field::MIN; value <= field::MAX; value += field::STEP)

namespace mamefont::mamec {

void Encoder::addFont(const BitmapFont &bmpFont) {
  if (options.verbose) {
    std::cout << "Adding font: " << bmpFont->familyName.c_str() << std::endl;
  }

  fontHeight = bmpFont->bodySize;
  ySpacing = bmpFont->ySpacing;
  for (const auto &bmpGlyph : bmpFont->glyphs) {
    addGlyph(bmpFont, bmpGlyph);
  }
}

void Encoder::addGlyph(const BitmapFont &bmpFont, const BitmapGlyph &bmpGlyph) {
  auto frags = bmpGlyph->bmp->toFragments(options.verticalFrag, options.msb1st);

  MameGlyph existingGlyph = std::make_shared<MameGlyphClass>(
      bmpGlyph->code, frags, bmpGlyph->width, bmpFont->bodySize,
      options.verticalFrag, options.msb1st,
      bmpFont->defaultXSpacing - bmpGlyph->leftAntiSpace,
      bmpGlyph->leftAntiSpace);

  // todo: delete
  // Detect glyph duplication
  for (auto &other : glyphs) {
    int otherCode = other.first;
    MameGlyph &otherGlyph = other.second;
    if (otherGlyph->fragments == frags) {
      if (options.verbose) {
        std::cout << "  Glyph duplication found: " << formatChar(bmpGlyph->code)
                  << " --> " << formatChar(otherCode) << std::endl;
      }

      existingGlyph->fragmentsSameAsCode = otherCode;
      break;
    }
  }

  glyphs[bmpGlyph->code] = existingGlyph;
}

void Encoder::encode() {
  if (options.verbose) {
    std::cout << "Encoding glyphs..." << std::endl;
    std::cout << "  Generating initial operations..." << std::endl;
  }

  for (auto &glyphPair : glyphs) {
    MameGlyph &glyph = glyphPair.second;

    // Generate operations for the glyph
    generateInitialOperations(glyph);
  }

  generateLut();

  replaceLDItoLUP();
}

static void dumpTree(const BufferState &state, int *nodeCount, int pos) {
  nodeCount[state->pos]++;
  if (state->pos != pos) {
    // todo: delete
    throw std::runtime_error("BufferState position mismatch: expected " +
                             std::to_string(pos) + ", got " +
                             std::to_string(state->pos));
  }
  for (const auto &nextPair : state->childState) {
    dumpTree(nextPair.second, nodeCount, pos + 1);
  }
}

void Encoder::generateInitialOperations(MameGlyph &glyph) {
  if (options.verbose) {
    std::cout << "    Generating operations for " << formatChar(glyph->code)
              << std::endl;
  }

  int numFrags = glyph->fragments.size();

  std::vector<fragment_t> compareMask(numFrags, 0xFF);
  if (!options.forceZeroPadding) {
    compareMask = glyph->createCompareMaskArray();

    if (options.verbose && options.verboseForCode == glyph->code) {
      std::cout << "      Compare mask: " << std::endl;
      size_t n = compareMask.size();
      for (size_t i = 0; i < n; i++) {
        if (i % 16 == 0) std::cout << "        ";
        std::cout << byteToHexStr(compareMask[i]) << " ";
        if ((i + 1) % 16 == 0 || i == n - 1) std::cout << std::endl;
      }
      std::cout << std::endl;
    }
  }

  int duplicatedCode = -1;
  size_t duplicatedSize = 0;
  for (auto &otherPair : glyphs) {
    MameGlyph &other = otherPair.second;
    auto otherSize = other->fragments.size();
    if (other->code == glyph->code) continue;
    if (otherSize < numFrags) {
      continue;
    }
    if (otherSize == numFrags && other->code > glyph->code) {
      continue;
    }

    auto otherPart = VecRef(other->fragments, 0, numFrags);
    if (!maskedEqual(glyph->fragments, otherPart, compareMask)) {
      continue;
    }

    if (otherSize > duplicatedSize) {
      duplicatedSize = otherSize;
      duplicatedCode = other->code;
    }
  }
  if (false && duplicatedCode >= 0) {
    // todo: Enable
    if (options.verbose) {
      std::cout << "    Fragment duplication found: " << formatChar(glyph->code)
                << " --> " << formatChar(duplicatedCode) << std::endl;
    }
    glyph->fragmentsSameAsCode = duplicatedCode;
  }

  StateQueue waitList;
  auto first = std::make_shared<BufferStateClass>(0, 0, nullptr);
  waitList.put(first, 0);

  BufferState bestGoal = nullptr;
  int bestGoalCost = DUMMY_COST;

  while (!waitList.empty()) {
    BufferState curr = waitList.popBest();

    if (curr->pos >= numFrags) {
      bestGoal = curr;
      bestGoalCost = curr->bestCost;
      break;
    }

    VecRef future(glyph->fragments, curr->pos, numFrags);
    VecRef mask(compareMask, curr->pos, numFrags);

    std::vector<Operation> oprs;
    TryContext ctx{oprs, curr, future, mask};
    tryLDI(ctx);
    tryXOR(ctx);
    tryRPT(ctx);
    trySFT(ctx);
    trySFI(ctx);
    tryCPY(ctx);
    tryCPX(ctx);

    for (const auto &opr : oprs) {
#if 0
      // todo delete checker code:
      if (!maskedEqual(opr->generated, future.slice(0, opr->generated.size()),
                       mask.slice(0, opr->generated.size()))) {
        throw std::runtime_error(
            std::string("Operation ") + mf::mnemonicOf(opr->op) +
            " generated fragments do not match future fragments for " +
            formatChar(glyph->code));
      }
#endif

      BufferState p = curr;
      for (fragment_t frag : opr->generated) {
        if (p->childState.contains(frag)) {
          p = p->childState[frag];
        } else {
          auto newState =
              std::make_shared<BufferStateClass>(p->pos + 1, frag, p);
          p->childState[frag] = newState;
          p = newState;
        }
      }

      int nextCost = curr->bestCost + opr->cost;
      if (nextCost < p->bestCost) {
        p->bestOpr = opr;
        p->bestPrev = curr;
        waitList.put(p, nextCost);
      }
    }

    if (options.verbose && options.verboseForCode == glyph->code) {
      int numNodes[numFrags + 1] = {0};
      dumpTree(first, numNodes, 0);
      for (int i = 0; i <= numFrags; i++) {
        printf("%3d ", numNodes[i]);
      }
      printf("\n");
    }
  }

  if (!bestGoal) {
    throw std::runtime_error("No solutions found for " +
                             formatChar(glyph->code));
  }

  BufferState p = bestGoal;
  std::vector<Operation> oprs;
  while (p->id != first->id) {
    oprs.insert(oprs.begin(), p->bestOpr);
    p = p->bestPrev;
  }

  if (options.verbose && options.verboseForCode == glyph->code) {
    std::vector<uint8_t> byteCode;
    for (const auto &opr : oprs) {
      byteCode.insert(byteCode.end(), opr->byteCode.begin(),
                      opr->byteCode.end());
    }
    std::cout << "    " << oprs.size() << " operations generated." << std::endl;
    glyph->report("      ");
    for (size_t i = 0; i < byteCode.size(); i++) {
      if (i % 16 == 0) {
        std::cout << "        ";
      }
      std::cout << byteToHexStr(byteCode[i]) << " ";
      if (i % 16 == 15 || i == byteCode.size() - 1) {
        std::cout << std::endl;
      }
    }
  }

  glyph->operations = oprs;
}

void Encoder::tryLDI(TryContext ctx) {
  ctx.oprs.push_back(makeLDI(ctx.future[0]));
}

void Encoder::tryXOR(TryContext ctx) {
  FOR_FIELD_VALUES(mf::XOR_POS, pos) {
    for (bool width2bit : {false, true}) {
      // This combination is reserved for other instruction
      if (width2bit && pos == 7) continue;

      fragment_t mask = (width2bit ? 0x03 : 0x01) << pos;
      fragment_t generated = ctx.state->lastFrag ^ mask;

      if (maskedEqual(generated, ctx.future[0], ctx.compareMask[0])) {
        ctx.oprs.push_back(makeXOR(pos, width2bit, generated));
        return;
      }
    }
  }
}

void Encoder::tryRPT(TryContext ctx) {
  int rptMax = std::min((size_t)mf::RPT_REPEAT_COUNT::MAX, ctx.future.size);
  int rptStep = mf::RPT_REPEAT_COUNT::STEP;
  fragment_t lastFrag = ctx.state->lastFrag;
  for (int rpt = 1; rpt <= rptMax; rpt += rptStep) {
    if (!maskedEqual(lastFrag, ctx.future[rpt - 1], ctx.compareMask[rpt - 1])) {
      break;
    }
    if (rpt >= mf::RPT_REPEAT_COUNT::MIN) {
      ctx.oprs.push_back(makeRPT(lastFrag, rpt));
    }
  }
}

void Encoder::trySFT(TryContext ctx) {
  for (bool right : {false, true}) {
    for (bool postSet : {false, true}) {
      FOR_FIELD_VALUES(mf::SFT_SIZE, size) {
        tryShiftCore(ctx, false, right, postSet, false, size, 1);
      }
    }
  }
}

void Encoder::trySFI(TryContext ctx) {
  for (bool right : {false, true}) {
    for (bool postSet : {false, true}) {
      // for (bool preShift : {false, true}) {
      int preShift = false;
      FOR_FIELD_VALUES(mf::SFI_PERIOD, period) {
        tryShiftCore(ctx, true, right, postSet, preShift, 1, period);
      }
      //}
    }
  }
}

void Encoder::tryShiftCore(TryContext ctx, bool isSFI, bool right, bool postSet,
                           bool preShift, int size, int period) {
  int rptMin = isSFI ? mf::SFI_REPEAT_COUNT::MIN : mf::SFT_REPEAT_COUNT::MIN;
  int rptMax = isSFI ? mf::SFI_REPEAT_COUNT::MAX : mf::SFT_REPEAT_COUNT::MAX;
  if (preShift) {
    rptMax = std::min(rptMax, ((int)ctx.future.size - 1) / period);
  } else {
    rptMax = std::min(rptMax, (int)ctx.future.size / period);
  }

  std::vector<fragment_t> generated;

  fragment_t modifier = (1 << size) - 1;
  if (right) modifier <<= (8 - size);
  if (!postSet) modifier = ~modifier;

  fragment_t work = ctx.state->lastFrag;
  bool changeDetected = false;

  int offset = 0;
  for (int rpt = (preShift ? 0 : 1); rpt <= rptMax; rpt++) {
    int startPhase = preShift ? (period - 1) : 0;
    for (int phase = startPhase; phase < period; phase++) {
      fragment_t lastWork = work;
      if (phase == period - 1) {
        if (right) {
          work >>= size;
        } else {
          work <<= size;
        }
        if (postSet) {
          work |= modifier;
        } else {
          work &= modifier;
        }
      }
      if (!maskedEqual(lastWork, work, ctx.compareMask[offset])) {
        changeDetected = true;
      }
      if (maskedEqual(work, ctx.future[offset], ctx.compareMask[offset])) {
        generated.push_back(work);
        offset++;
      } else {
        return;
      }
    }
    if (rpt >= rptMin && changeDetected) {
      if (isSFI) {
        ctx.oprs.push_back(
            makeSFI(right, postSet, preShift, period, rpt, generated));
      } else {
        ctx.oprs.push_back(makeSFT(right, postSet, size, rpt, generated));
      }
    }
  }
}

void Encoder::tryCPY(TryContext ctx) {
  std::vector<fragment_t> pastBuff;
  const int pastLen = mf::CPY_LENGTH::MAX + mf::CPY_OFFSET::MAX;
  pastBuff.resize(pastLen);
  ctx.state->copyPastTo(pastBuff, pastLen, pastLen);
  FOR_FIELD_VALUES(mf::CPY_LENGTH, length) {
    if (length > ctx.future.size) break;
    FOR_FIELD_VALUES(mf::CPY_OFFSET, offset) {
      int iPastFrom = pastBuff.size() - offset - length;
      int iPastTo = iPastFrom + length;
      VecRef pastRef(pastBuff, iPastFrom, iPastTo);
      VecRef futureRef = ctx.future.slice(0, length);
      VecRef maskRef = ctx.compareMask.slice(0, length);

      for (bool byteReverse : {false, true}) {
        // These combinations are reserved for other instructions
        if (!byteReverse && length == 1 && offset == 0) continue;
        if (byteReverse && length == 1) continue;

        uint8_t cpxFlags = mf::CPX_BYTE_REVERSE::place(byteReverse);
        if (maskedEqual(pastRef, futureRef, maskRef, cpxFlags)) {
          ctx.oprs.push_back(
              makeCPY(offset, length, byteReverse, pastRef.toVector(cpxFlags)));
          goto nextLength;
        }
      }
    }
  nextLength:
  }
}

void Encoder::tryCPX(TryContext ctx) {
  if (options.noCpx) return;  // Skip CPX if disabled

  std::vector<fragment_t> pastBuff;
  const int pastLen = mf::CPX_OFFSET_MAX;
  pastBuff.resize(pastLen);
  ctx.state->copyPastTo(pastBuff, pastLen, pastLen);

  FOR_FIELD_VALUES(mf::CPX_LENGTH, length) {
    if (length > ctx.future.size) break;

    VecRef futureRef = ctx.future.slice(0, length);
    VecRef maskRef = ctx.compareMask.slice(0, length);

    int ofstMin = mf::CPX_OFFSET_MIN;
    int ofstMax = mf::CPX_OFFSET_MAX;
    int ofstStep = mf::CPX_OFFSET_STEP;
    for (int offset = ofstMin; offset <= ofstMax; offset += ofstStep) {
      int iPastFrom = pastBuff.size() - offset;
      int iPastTo = iPastFrom + length;
      if (iPastTo > pastBuff.size()) continue;
      if (iPastFrom < -length) break;

      VecRef pastRef(pastBuff, iPastFrom, iPastTo);

      for (bool byteReverse : {false, true}) {
        for (bool bitReverse : {false, true}) {
          for (bool inverse : {false, true}) {
            uint8_t cpxFlags = 0;
            cpxFlags |= mf::CPX_BYTE_REVERSE::place(byteReverse);
            cpxFlags |= mf::CPX_BIT_REVERSE::place(bitReverse);
            cpxFlags |= mf::CPX_INVERSE::place(inverse);
            if (maskedEqual(pastRef, futureRef, maskRef, cpxFlags)) {
              ctx.oprs.push_back(makeCPX(offset, length, cpxFlags,
                                         pastRef.toVector(cpxFlags)));
              goto nextLength;
            }
          }
        }
      }
    }
  nextLength:
  }
}

void Encoder::generateLut() {
  if (options.verbose) {
    std::cout << "Generating LUT..." << std::endl;
  }

  // count how many times each fragment is used in LDI operations
  std::map<fragment_t, int> lutMap;
  for (const auto &glyphPair : glyphs) {
    const MameGlyph &glyph = glyphPair.second;
    for (const auto &opr : glyph->operations) {
      if (opr->op == mf::Operator::LDI) {
        fragment_t frag = opr->generated[0];
        if (lutMap.find(frag) == lutMap.end()) {
          lutMap[frag] = 1;
        } else {
          lutMap[frag] += 1;
        }
      }
    }
  }

  // Sort the LUT by usage count in descending order
  std::vector<std::pair<int, fragment_t>> sortedLut;
  for (const auto &kv : lutMap) {
    sortedLut.push_back({kv.second, kv.first});
  }
  std::sort(sortedLut.begin(), sortedLut.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });

  // Create the final LUT, keeping only the fragments
  int n = 0;
  lut.clear();
  for (const auto &kv : sortedLut) {
    lut.push_back(kv.second);
    if (++n >= mf::MAX_LUT_SIZE) break;
  }
}

void Encoder::replaceLDItoLUP() {
  if (options.verbose) {
    std::cout << "Replacing LDI with LUP/LUD..." << std::endl;
  }

  for (auto &glyphPair : glyphs) {
    MameGlyph &glyph = glyphPair.second;
    int n = glyph->operations.size();
    for (int i = 0; i < n; i++) {
      auto &opr = glyph->operations[i];
      if (opr->op == mf::Operator::LDI) {
        fragment_t frag = opr->generated[0];
        int index = findFragmentFromLUT(frag);
        if (index >= 0) {
          glyph->operations[i] = makeLUP(index, frag);
        }
      }
    }
  }
}

int Encoder::findFragmentFromLUT(fragment_t frag) {
  int n = lut.size();
  for (int j = 0; j < n; j++) {
    if (lut[j] == frag) {
      return j;
    }
  }
  return -1;  // Not found
}

void Encoder::generateBlob() {
  if (options.verbose) {
    std::cout << "Generating blob..." << std::endl;
  }

  bool shrinkedGlyphTable = false;

  std::vector<uint8_t> bytecodes;
  for (const auto &glyphPair : glyphs) {
    const MameGlyph &glyph = glyphPair.second;
    if (glyph->fragmentsSameAsCode >= 0) {
      const MameGlyph &dupGlyph = glyphs[glyph->fragmentsSameAsCode];
      glyph->entryPoint = dupGlyph->entryPoint;
      glyph->byteCodeSize = dupGlyph->byteCodeSize;
      continue;
    }
    glyph->entryPoint = bytecodes.size();
    for (const auto &opr : glyph->operations) {
      bytecodes.insert(bytecodes.end(), opr->byteCode.begin(),
                       opr->byteCode.end());
    }
    if (shrinkedGlyphTable) {
      while (bytecodes.size() % 2 != 0) {
        bytecodes.push_back(mf::baseCodeOf(mf::Operator::ABO));
      }
    }
    glyph->byteCodeSize = bytecodes.size() - glyph->entryPoint;

    if (options.verbose && options.verboseForCode == glyph->code) {
      std::cout << "  Bytecode generated for glyph " << formatChar(glyph->code)
                << ":" << std::endl
                << "    Entry Point: " << glyph->entryPoint << std::endl;
      glyph->report("    ");
    }
  }

  if (lut.size() % 2 != 0) {
    if (options.verbose) {
      std::cout << "  LUT size is odd, adding a dummy entry." << std::endl;
    }
    lut.push_back(0x00);
  }

  std::vector<int> codes;
  for (const auto &glyphPair : glyphs) {
    codes.push_back(glyphPair.first);
  }
  std::sort(codes.begin(), codes.end());

  int firstCode = codes.front();
  int lastCode = codes.back();

  int maxGlyphWidth = 1;
  for (const auto &glyphPair : glyphs) {
    const MameGlyph &glyph = glyphPair.second;
    if (glyph->width > maxGlyphWidth) {
      maxGlyphWidth = glyph->width;
    }
  }

  uint8_t version = 1;

  uint8_t fontFlags = 0;
  fontFlags |= mf::FONT_FLAG_VERTICAL_FRAGMENT::place(options.verticalFrag);
  fontFlags |= mf::FONT_FLAG_MSB1ST::place(options.msb1st);
  fontFlags |= mf::FONT_FLAG_SHRINKED_GLYPH_TABLE::place(shrinkedGlyphTable);

  uint8_t fontDimension0 = mf::FONT_DIM_FONT_HEIGHT::place(fontHeight);
  uint8_t fontDimension1 = mf::FONT_DIM_Y_SPACING::place(ySpacing);
  uint8_t fontDimension2 = mf::FONT_DIM_MAX_GLYPH_WIDTH::place(maxGlyphWidth);

  // Font Header
  blob.clear();
  blob.push_back(version);
  blob.push_back(fontFlags);
  blob.push_back(firstCode);
  blob.push_back(lastCode - firstCode);
  blob.push_back(lut.size() - 1);
  blob.push_back(fontDimension0);
  blob.push_back(fontDimension1);
  blob.push_back(fontDimension2);

  // Glyph Table
  for (int code = firstCode; code <= lastCode; code++) {
    auto it = glyphs.find(code);
    if (it == glyphs.end() || it->second->width == 0) {
      // Dummy Entry for missing glyphs
      if (shrinkedGlyphTable) {
        blob.push_back(0xFF);
        blob.push_back(0xFF);
      } else {
        blob.push_back(0xFF);
        blob.push_back(0xFF);
        blob.push_back(0xFF);
        blob.push_back(0xFF);
      }
    } else {
      const MameGlyph &glyph = it->second;
      if (shrinkedGlyphTable) {
        uint8_t glyphDim = 0;
        glyphDim |= mf::GLYPH_SHRINKED_DIM_WIDTH::place(glyph->width);
        glyphDim |= mf::GLYPH_SHRINKED_DIM_X_SPACING::place(glyph->xSpacing);
        glyphDim |= mf::GLYPH_SHRINKED_DIM_X_NEGATIVE_OFFSET::place(
            glyph->xNegativeOffset);
        blob.push_back(glyph->entryPoint / 2);
        blob.push_back(glyphDim);
      } else {
        uint8_t glyphDim0 = 0;
        glyphDim0 |= mf::GLYPH_DIM_WIDTH::place(glyph->width);
        uint8_t glyphDim1 = 0;
        glyphDim1 |= mf::GLYPH_DIM_X_SPACING::place(glyph->xSpacing);
        glyphDim1 |=
            mf::GLYPH_DIM_X_NEGATIVE_OFFSET::place(glyph->xNegativeOffset);
        blob.push_back(glyph->entryPoint & 0xFF);
        blob.push_back(glyph->entryPoint >> 8);
        blob.push_back(glyphDim0);
        blob.push_back(glyphDim1);
      }
    }
  }

  blob.insert(blob.end(), lut.begin(), lut.end());

  blob.insert(blob.end(), bytecodes.begin(), bytecodes.end());

  if (options.verbose) {
    std::cout << "  Blob size: " << blob.size() << " bytes" << std::endl;
  }
}

}  // namespace mamefont::mamec
