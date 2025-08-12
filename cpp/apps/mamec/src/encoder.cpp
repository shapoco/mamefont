#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "mamec/bitmap_glyph.hpp"
#include "mamec/buffer_state.hpp"
#include "mamec/encoder.hpp"
#include "mamec/glyph_object.hpp"
#include "mamec/gray_bitmap.hpp"
#include "mamec/state_queue.hpp"
#include "mamec/vec_ref.hpp"

#define FOR_FIELD_VALUES(field, value) \
  for (int value = field::MIN; value <= field::MAX; value += field::STEP)

namespace mamefont::mamec {

static void dumpSearchTree(const BufferState &state, int *nodeCount, int length,
                           int pos);

void Encoder::addFont(const BitmapFont &bmpFont) {
  if (options.verbose) {
    std::cout << "Adding font: " << bmpFont->familyName.c_str() << std::endl;
  }

  glyphHeight = bmpFont->bodySize;
  ySpacing = bmpFont->ySpacing;
  for (const auto &bmpGlyph : bmpFont->glyphs) {
    addGlyph(bmpFont, bmpGlyph);
  }
}

void Encoder::addGlyph(const BitmapFont &bmpFont, const BitmapGlyph &bmpGlyph) {
  auto frags = bmpGlyph->bmp->toFragments(options.verticalFrag, options.msb1st);

  GlyphObject existingGlyph = std::make_shared<GlyphObjectClass>(
      bmpGlyph->code, frags, bmpGlyph->width, bmpFont->bodySize,
      options.verticalFrag, options.msb1st,
      bmpFont->defaultXSpacing - bmpGlyph->leftAntiSpace,
      bmpGlyph->leftAntiSpace);

  // todo: delete
  // Detect glyph duplication
  for (auto &other : glyphs) {
    int otherCode = other.first;
    GlyphObject &otherGlyph = other.second;
    if (otherGlyph->fragments == frags) {
      if (options.verbose) {
        std::cout << "  Glyph duplication found: " << c2s(bmpGlyph->code)
                  << " --> " << c2s(otherCode) << std::endl;
      }

      existingGlyph->fragmentsSameAsCode = otherCode;
      break;
    }
  }

  glyphs[bmpGlyph->code] = existingGlyph;
}

void Encoder::encode() {
  if (options.verbose) {
    std::cout << "Generating initial fragment table..." << std::endl;
  }
  generateInitialFragTable();
  if (options.verbose) {
    dumpByteArray(fragTable, "  ");
    std::cout << "  (" << fragTable.size() << " entries)" << std::endl;
  }

  if (options.verbose) {
    std::cout << "Encoding glyphs..." << std::endl;
  }
  for (auto &glyphPair : glyphs) {
    GlyphObject &glyph = glyphPair.second;
    if (options.verbose) {
      std::cout << "  Generating operations for " << c2s(glyph->code)
                << std::endl;
    }
    bool v = options.verbose && options.verboseForCode == glyph->code;
    generateInitialOperations(glyph, v, "    ");
  }

  if (options.verbose) {
    std::cout << "Regenerating fragment table..." << std::endl;
  }
  generateFullFragTable();
  if (options.verbose) {
    dumpByteArray(fragTable, "  ");
    std::cout << "  (" << fragTable.size() << " entries)" << std::endl;
  }

  if (options.verbose) {
    std::cout << "Optimizing Fragment Table..." << std::endl;
  }
  optimizeFragmentTable();
  if (options.verbose) {
    dumpByteArray(fragTable, "  ");
    std::cout << "  (" << fragTable.size() << " entries)" << std::endl;
  }

  if (options.verbose) {
    std::cout << "Replacing LDI with LUP/LUD..." << std::endl;
  }
  replaceLDItoLUP(options.verbose, "  ");

  if (options.verbose) {
    std::cout << "Encode finished." << std::endl;
  }
}

void Encoder::generateInitialFragTable() {
  // count how many times each fragment is used in LDI operations
  std::map<frag_t, int> fragCountMap;
  for (const auto &glyphPair : glyphs) {
    for (frag_t frag : glyphPair.second->fragments) {
      if (fragCountMap.contains(frag)) {
        fragCountMap[frag] += 1;
      } else {
        fragCountMap[frag] = 1;
      }
    }
  }

  generateFragTableFromCountMap(fragCountMap, mf::MAX_FRAGMENT_TABLE_SIZE / 2);
}

void Encoder::generateInitialOperations(GlyphObject &glyph, bool verbose,
                                        std::string indent) {
  int numFrags = glyph->fragments.size();

  std::vector<frag_t> compareMask(numFrags, 0xFF);
  if (!options.forceZeroPadding) {
    compareMask = glyph->createCompareMaskArray();

    if (verbose) {
      std::cout << indent << "Compare mask: " << std::endl;
      dumpByteArray(compareMask, indent + "  ");
    }
  }

  int duplicatedCode = -1;
  size_t duplicatedSize = 0;
  for (auto &otherPair : glyphs) {
    GlyphObject &other = otherPair.second;
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
    if (verbose) {
      std::cout << indent
                << "Fragment duplication found: " << c2s(glyph->code)
                << " --> " << c2s(duplicatedCode) << std::endl;
    }
    glyph->fragmentsSameAsCode = duplicatedCode;
  }

  std::vector<std::vector<int>> scoreBoard(numFrags + 1,
                                           std::vector<int>(256, DUMMY_COST));

  BufferState goalState = nullptr;

  StateQueue waitList;
  auto first = std::make_shared<BufferStateClass>(0, 0, nullptr);
  waitList.put(first, 0);

  int treeLeafCount[numFrags + 1] = {0};
  char treeStateStr[numFrags + 2] = {' '};
  treeStateStr[numFrags + 1] = '\0';

  if (verbose) {
    std::cout << indent << "Searching solution..." << std::endl;
  }
  while (!waitList.empty()) {
    BufferState curr = waitList.popBest();

    if (curr->pos >= numFrags) {
      if (curr->pos > numFrags) {
        throw std::runtime_error("Search overrun");
      }
      goalState = curr;
      break;
    }

    VecRef future(glyph->fragments, curr->pos, numFrags);
    VecRef mask(compareMask, curr->pos, numFrags);

    std::vector<Operation> oprs;
    TryContext ctx{glyph->code, oprs, curr, future, mask};
    tryLUP(ctx);
    tryXOR(ctx);
    tryRPT(ctx);
    trySFT(ctx);
    trySFI(ctx);
    tryCPY(ctx);
    tryCPX(ctx);

    bool treeChanged = false;
    for (const auto &opr : oprs) {
#if 0
      // todo delete checker code:
      if (!maskedEqual(opr->output, future.slice(0, opr->output.size()),
                       mask.slice(0, opr->output.size()))) {
        throw std::runtime_error(
            std::string("Operation ") + mf::mnemonicOf(opr->op) +
            " generated fragments do not match future fragments for " +
            c2s(glyph->code));
      }
#endif

      BufferState p = curr;
      for (frag_t frag : opr->output) {
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
      int otherCost = scoreBoard[p->pos][p->lastFrag];
      if (nextCost < p->bestCost && nextCost <= otherCost) {
        p->bestOpr = opr;
        p->bestPrev = curr;
        waitList.put(p, nextCost);
        scoreBoard[p->pos][p->lastFrag] = nextCost;
        treeChanged = true;
      }
    }

    if (treeChanged && verbose) {
      memset(treeLeafCount, 0, sizeof(treeLeafCount));
      dumpSearchTree(first, treeLeafCount, numFrags + 1, 0);
      bool strChanged = false;
      for (int i = 0; i < numFrags + 1; i++) {
        char c = '.';
        if (treeLeafCount[i] > 0) {
          int digit = log2(treeLeafCount[i]) / log2(2) + 1;
          if (digit > 9) digit = 9;
          c = '0' + digit;
        }
        if (treeStateStr[i] != c) {
          treeStateStr[i] = c;
          strChanged = true;
        }
      }
      if (strChanged) {
        std::cout << indent << "  [" << treeStateStr << "]" << std::endl;
      }
    }
  }

  if (!goalState) {
    throw std::runtime_error("No solutions found for " +
                             c2s(glyph->code));
  }

  BufferState p = goalState;
  std::vector<Operation> oprs;
  while (p->id != first->id) {
    oprs.insert(oprs.begin(), p->bestOpr);
    p = p->bestPrev;
  }

  if (options.verbose && options.verboseForCode == glyph->code) {
    std::vector<uint8_t> byteCode;
    for (const auto &opr : oprs) {
      opr->writeCodeTo(byteCode);
    }
    std::cout << indent << oprs.size() << " operations, " << byteCode.size()
              << " bytes generated." << std::endl;
    dumpByteArray(byteCode, indent + "  ");
  }

  glyph->operations = oprs;
}

void Encoder::tryLUP(TryContext ctx) {
  for (int frag = 0; frag <= 0xFF; frag++) {
    if (maskedEqual(frag, ctx.future[0], ctx.compareMask[0])) {
      int index = reverseLookup(frag);
      if (index >= 0) {
        ctx.oprs.push_back(makeLUP(index, frag));
      } else {
        if (ldiFrags.contains(frag)) {
          ldiFrags[frag]++;
        } else {
          ldiFrags[frag] = 1;
        }
        int addCost = ldiFrags[frag] > 0 ? 1 : 0;
        ctx.oprs.push_back(makeLDI(frag, addCost));
      }
    }
  }
}

void Encoder::tryXOR(TryContext ctx) {
  FOR_FIELD_VALUES(mf::XOR::Pos, pos) {
    for (bool width2bit : {false, true}) {
      // This combination is reserved for other instruction
      if (width2bit && pos == 7) continue;

      frag_t mask = (width2bit ? 0x03 : 0x01) << pos;
      frag_t output = ctx.state->lastFrag ^ mask;

      if (maskedEqual(output, ctx.future[0], ctx.compareMask[0])) {
        ctx.oprs.push_back(makeXOR(pos, width2bit, output));
        return;
      }
    }
  }
}

void Encoder::tryRPT(TryContext ctx) {
  int rptMax = std::min((size_t)mf::RPT::RepeatCount::MAX, ctx.future.size);
  int rptStep = mf::RPT::RepeatCount::STEP;
  frag_t lastFrag = ctx.state->lastFrag;
  for (int rpt = 1; rpt <= rptMax; rpt += rptStep) {
    if (!maskedEqual(lastFrag, ctx.future[rpt - 1], ctx.compareMask[rpt - 1])) {
      break;
    }
    if (rpt >= mf::RPT::RepeatCount::MIN) {
      ctx.oprs.push_back(makeRPT(lastFrag, rpt));
    }
  }
}

void Encoder::trySFT(TryContext ctx) {
  frag_t last = ctx.state->lastFrag;
  if (last == ctx.future[0]) {
    // If no shift is needed, return early
    return;
  }
  for (bool postSet : {false, true}) {
    if ((last == 0x00 && !postSet) || (last == 0xFF && postSet)) {
      // If the last fragment is all zeros or all ones, no shift can change it
      continue;
    }
    FOR_FIELD_VALUES(mf::SFT::Size, size) {
      for (bool right : {false, true}) {
        if (tryShiftCore(ctx, false, right, postSet, false, size, 1)) {
          return;
        }
      }
    }
  }
}

void Encoder::trySFI(TryContext ctx) {
  if (options.noSfi) return;

  frag_t last = ctx.state->lastFrag;
  for (bool postSet : {false, true}) {
    if ((last == 0x00 && !postSet) || (last == 0xFF && postSet)) {
      // If the last fragment is all zeros or all ones, no shift can change it
      continue;
    }
    for (bool preShift : {false, true}) {
      FOR_FIELD_VALUES(mf::SFI::Period, period) {
        for (bool right : {false, true}) {
          if (tryShiftCore(ctx, true, right, postSet, preShift, 1, period)) {
            return;
          }
        }
      }
    }
  }
}

bool Encoder::tryShiftCore(TryContext ctx, bool isSFI, bool right, bool postSet,
                           bool preShift, int size, int period) {
  int rptMin = isSFI ? mf::SFI::RepeatCount::MIN : mf::SFT::RepeatCount::MIN;
  int rptMax = isSFI ? mf::SFI::RepeatCount::MAX : mf::SFT::RepeatCount::MAX;
  if (preShift) {
    rptMax = std::min(rptMax, ((int)ctx.future.size - 1) / period);
  } else {
    rptMax = std::min(rptMax, (int)ctx.future.size / period);
  }

  std::vector<frag_t> output;

  frag_t modifier = (1 << size) - 1;
  if (right) modifier <<= (8 - size);
  if (!postSet) modifier = ~modifier;

  frag_t work = ctx.state->lastFrag;
  bool changeDetected = false;

  int offset = 0;
  for (int rpt = (preShift ? 0 : 1); rpt <= rptMax; rpt++) {
    int startPhase = (rpt == 0 && preShift) ? (period - 1) : 0;
    for (int phase = startPhase; phase < period; phase++) {
      frag_t lastWork = work;
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
        output.push_back(work);
        offset++;
      } else {
        return false;
      }
    }
    if (rpt >= rptMin && changeDetected) {
      if (isSFI) {
        ctx.oprs.push_back(
            makeSFI(right, postSet, preShift, period, rpt, output));
      } else {
        ctx.oprs.push_back(makeSFT(right, postSet, size, rpt, output));
      }
    }
  }
  return changeDetected;
}

void Encoder::tryCPY(TryContext ctx) {
  std::vector<frag_t> pastBuff;
  const int pastLen = mf::CPY::Length::MAX + mf::CPY::Offset::MAX;
  pastBuff.resize(pastLen);
  ctx.state->copyPastTo(pastBuff, pastLen, pastLen);
  FOR_FIELD_VALUES(mf::CPY::Length, length) {
    if (length > ctx.future.size) break;
    FOR_FIELD_VALUES(mf::CPY::Offset, offset) {
      int iPastFrom = pastBuff.size() - offset - length;
      int iPastTo = iPastFrom + length;
      VecRef pastRef(pastBuff, iPastFrom, iPastTo);
      VecRef futureRef = ctx.future.slice(0, length);
      VecRef maskRef = ctx.compareMask.slice(0, length);

      for (bool byteReverse : {false, true}) {
        // These combinations are reserved for other instructions
        if (!byteReverse && length == 1 && offset == 0) continue;
        if (byteReverse && length == 1) continue;

        uint8_t cpxFlags = mf::CPX::ByteReverse::place(byteReverse);
        if (maskedEqual(pastRef, futureRef, maskRef, cpxFlags)) {
          ctx.oprs.push_back(
              makeCPY(offset, length, byteReverse, pastRef.toVector(cpxFlags)));
          goto nextLength;
        }
      }
    }
  nextLength:
    continue;  // dummy op to suppress warning
  }
}

void Encoder::tryCPX(TryContext ctx) {
  if (options.noCpx) return;  // Skip CPX if disabled

  std::vector<frag_t> pastBuff;
  const int pastLen = mf::CPX::Offset::MAX;
  pastBuff.resize(pastLen);
  ctx.state->copyPastTo(pastBuff, pastLen, pastLen);

  FOR_FIELD_VALUES(mf::CPX::Length, length) {
    if (length > ctx.future.size) break;

    VecRef futureRef = ctx.future.slice(0, length);
    VecRef maskRef = ctx.compareMask.slice(0, length);

    bool found = false;

    FOR_FIELD_VALUES(mf::CPX::Offset, offset) {
      int iPastFrom = pastBuff.size() - offset;
      int iPastTo = iPastFrom + length;
      if (iPastTo > pastBuff.size()) continue;
      if (iPastFrom < -length) break;

      VecRef pastRef(pastBuff, iPastFrom, iPastTo);

      for (bool byteReverse : {false, true}) {
        for (bool bitReverse : {false, true}) {
          for (bool inverse : {false, true}) {
            uint8_t cpxFlags = 0;
            cpxFlags |= mf::CPX::ByteReverse::place(byteReverse);
            cpxFlags |= mf::CPX::BitReverse::place(bitReverse);
            cpxFlags |= mf::CPX::Inverse::place(inverse);
            if (maskedEqual(pastRef, futureRef, maskRef, cpxFlags)) {
              ctx.oprs.push_back(makeCPX(offset, length, cpxFlags,
                                         pastRef.toVector(cpxFlags)));
              found = true;
              goto nextLength;
            }
          }
        }
      }
    }
  nextLength:
    if (!found) return;
  }
}

static void dumpSearchTree(const BufferState &state, int *nodeCount, int length,
                           int pos) {
#if 0
  if (state->pos != pos || pos >= length) {
    throw std::runtime_error("BufferState position mismatch: expected " +
                             std::to_string(pos) + ", got " +
                             std::to_string(state->pos));
  }
#endif
  nodeCount[state->pos]++;
  for (const auto &nextPair : state->childState) {
    dumpSearchTree(nextPair.second, nodeCount, length, pos + 1);
  }
}

void Encoder::generateFullFragTable() {
  // count how many times each fragment is used in LDI operations
  std::map<frag_t, int> fragCountMap;
  for (const auto &glyphPair : glyphs) {
    const GlyphObject &glyph = glyphPair.second;
    for (const auto &opr : glyph->operations) {
      if (opr->op == mf::Operator::LDI || opr->op == mf::Operator::LUP) {
        frag_t frag = opr->output[0];
        if (fragCountMap.find(frag) == fragCountMap.end()) {
          fragCountMap[frag] = 1;
        } else {
          fragCountMap[frag] += 1;
        }
      }
    }
  }

  generateFragTableFromCountMap(fragCountMap, mf::MAX_FRAGMENT_TABLE_SIZE);

  fixLUPIndex();
}

void Encoder::generateFragTableFromCountMap(std::map<frag_t, int> &countMap,
                                            int tableSize) {
  // Sort the Fragment Table by usage count in descending order
  std::vector<std::pair<int, frag_t>> sortedCount;
  for (const auto &kv : countMap) {
    sortedCount.push_back({kv.second, kv.first});
  }
  std::sort(sortedCount.begin(), sortedCount.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });

  // Generate the Fragment Table
  int n = 0;
  fragTable.clear();
  for (const auto &kv : sortedCount) {
    fragTable.push_back(kv.second);
    if (++n >= tableSize) break;
  }
}

void Encoder::optimizeFragmentTable() {
  // Detecte frequent sequences of two fragments
  std::map<uint16_t, int> sequenceCountMap;
  for (auto frag1 : fragTable) {
    for (auto frag2 : fragTable) {
      sequenceCountMap[(frag1 << 8) | frag2] = 0;
    }
  }
  for (const auto &glyphPair : glyphs) {
    const GlyphObject &glyph = glyphPair.second;
    int frag1 = -1;
    for (const auto &opr2 : glyph->operations) {
      int frag2 = -1;
      if (opr2->output.size() == 1 && reverseLookup(opr2->output[0]) >= 0) {
        frag2 = opr2->output[0];
      }
      if (frag1 >= 0 && frag2 >= 0) {
        uint16_t sequence = (frag1 << 8) | frag2;
        sequenceCountMap[sequence]++;
      }
      frag1 = frag2;
    }
  }

  // Sort sequences by usage count in descending order
  std::vector<std::pair<uint16_t, int>> mostFreqSeqs;
  for (const auto &kv : sequenceCountMap) {
    mostFreqSeqs.push_back({kv.first, kv.second});
  }
  std::sort(mostFreqSeqs.begin(), mostFreqSeqs.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });

  std::vector<std::vector<frag_t>> sequences;
  int seqSize = 0;
  int numFrozen = 0;
  bool headFrozen = false;
  while (!mostFreqSeqs.empty()) {
    auto seqPair = mostFreqSeqs.back();
    mostFreqSeqs.pop_back();

    uint16_t seq = seqPair.first;
    frag_t frag1 = (seq >> 8) & 0xFF;
    frag_t frag2 = seq & 0xFF;

    int frag1index = reverseLookup(frag1);
    int frag2index = reverseLookup(frag2);

    if (frag1index >= 0 && frag2index < 0) {
      int n = 0;
      for (auto &seq : sequences) {
        n += seq.size();
        if (seq[0] == frag2) {
          if (n <= numFrozen) {
            // remove from Fragment Table
            fragTable.erase(fragTable.begin() + frag2index);
            seq.insert(seq.begin(), frag1);
            seqSize += 1;
          }
          break;
        }
      }
    } else if (frag1index < 0 && frag2index >= 0) {
      int n = 0;
      for (auto &seq : sequences) {
        n += seq.size();
        if (seq.back() == frag1) {
          if (n <= numFrozen) {
            // remove from Fragment Table
            fragTable.erase(fragTable.begin() + frag1index);
            seq.push_back(frag2);
            seqSize += 1;
          }
          break;
        }
      }
    } else if (frag1index >= 0 && frag2index >= 0) {
      if (frag1 == frag2) {
        fragTable.erase(fragTable.begin() + frag1index);
        sequences.push_back({frag1});
        seqSize += 1;
      } else {
        fragTable.erase(fragTable.begin() + std::max(frag1index, frag2index));
        fragTable.erase(fragTable.begin() + std::min(frag1index, frag2index));
        sequences.push_back({frag1, frag2});
        seqSize += 2;
      }
    } else {
      int seq1index = -1;
      int n = 0;
      for (int i = 0; i < sequences.size(); i++) {
        n += sequences[i].size();
        if (n > numFrozen) break;
        if (sequences[i].back() == frag1) {
          seq1index = i;
          break;
        }
      }
      int seq2index = -1;
      for (int i = 0; i < sequences.size(); i++) {
        n += sequences[i].size();
        if (n > numFrozen) break;
        if (sequences[i][0] == frag2) {
          seq2index = i;
          break;
        }
      }
      if (seq1index >= 0 && seq2index >= 0 && seq1index != seq2index) {
        // Merge sequences
        auto seq1 = sequences[seq1index];
        auto seq2 = sequences[seq2index];
        int minIndex = std::min(seq1index, seq2index);
        int maxIndex = std::max(seq1index, seq2index);
        sequences.erase(sequences.begin() + maxIndex);
        sequences.erase(sequences.begin() + minIndex);
        seq1.insert(seq1.end(), seq2.begin(), seq2.end());
        sequences.insert(sequences.begin() + minIndex, seq1);
      }
    }

    if (!headFrozen && seqSize >= mf::LUD::Index::MAX) {
      headFrozen = true;
    }
  }

  std::vector<frag_t> newTable;
  for (const auto &seq : sequences) {
    newTable.insert(newTable.end(), seq.begin(), seq.end());
  }
  newTable.insert(newTable.end(), fragTable.begin(), fragTable.end());
  fragTable = std::move(newTable);

  fixLUPIndex();
}

void Encoder::fixLUPIndex() {
  // fix existing LUP operations
  for (auto &glyphPair : glyphs) {
    GlyphObject &glyph = glyphPair.second;
    for (int i = 0; i < glyph->operations.size(); i++) {
      auto &opr = glyph->operations[i];
      if (opr->op == mf::Operator::LUP) {
        int newIndex = reverseLookup(opr->output[0]);
        if (newIndex >= 0) {
          glyph->operations[i] = makeLUP(newIndex, opr->output[0]);
        } else {
          glyph->operations[i] = makeLDI(opr->output[0], 0);
          std::cerr << "  *WARNING: LUP unexpectedly replaced with LDI for "
                    << c2s(glyph->code) << ", since fragment 0x"
                    << byteToHexStr(opr->output[0])
                    << " not found in fragment table." << std::endl;
        }
      }
    }
  }
}

void Encoder::replaceLDItoLUP(bool verbose, std::string indent) {
  int numReplacedOps = 0;
  for (auto &glyphPair : glyphs) {
    GlyphObject &glyph = glyphPair.second;

    // replace single output instructions with LUP as possible
    for (int i = 0; i < glyph->operations.size(); i++) {
      auto &opr = glyph->operations[i];
      if (opr->output.size() == 1) {
        frag_t frag = opr->output[0];
        int index = reverseLookup(frag);
        if (index >= 0) {
          glyph->operations[i] = makeLUP(index, frag);
        }
      }
    }

    // replace pair of LUP with LUD if possible
    int index1 = -1;
    frag_t frag1 = 0x00;
    for (int i = 0; i < glyph->operations.size(); i++) {
      auto &opr = glyph->operations[i];
      int index2 = -1;
      frag_t frag2 = 0x00;
      if (opr->op == mf::Operator::LUP) {
        index2 = mf::LUP::Index::read(opr->code[0]);
        frag2 = opr->output[0];
      }
      if (index1 >= 0 && index2 >= 0 && index1 <= mf::LUD::Index::MAX &&
          (index1 == index2 || index1 + 1 == index2)) {
        int step = index2 - index1;
        glyph->operations[i - 1] = makeLUD(index1, step, frag1, frag2);
        glyph->operations.erase(glyph->operations.begin() + i);
        i--;           // Adjust index after removal
        index2 = -1;   // Reset index2 to avoid double replacement
        frag2 = 0x00;  // Reset frag2 to avoid double replacement
        numReplacedOps++;
      }
      index1 = index2;
      frag1 = frag2;
    }
  }

  if (verbose) {
    std::cout << indent << "Totally " << (numReplacedOps * 2)
              << " instructions replaced with " << numReplacedOps << " LUD ."
              << std::endl;
  }
}

int Encoder::reverseLookup(frag_t frag) {
  int n = fragTable.size();
  for (int j = 0; j < n; j++) {
    if (fragTable[j] == frag) {
      return j;
    }
  }
  return -1;  // Not found
}

void Encoder::generateBlob() {
  if (options.verbose) {
    std::cout << "Generating blob..." << std::endl;
  }

  // Determine format of Glyph Table
  std::map<std::string, bool> largeFontReasons;
  std::map<std::string, bool> proportionalReasons;
  int lastWidth = -1;
  int lastXSpacing = -1;
  int nextEntryPoint = 0;
  for (const auto &glyphPair : glyphs) {
    const GlyphObject &glyph = glyphPair.second;

    // Check glyph dimensions
    if (!mf::SmallGlyphDimension::GlyphWidth::inRange(glyph->width)) {
      largeFontReasons["glyphWidth"] = true;
    }
    if (!mf::SmallGlyphDimension::XSpacing::inRange(glyph->xSpacing)) {
      largeFontReasons["xSpacing"] = true;
    }
    if (!mf::SmallGlyphDimension::XStepBack::inRange(glyph->xStepBack)) {
      largeFontReasons["xStepBack"] = true;
    }
    if (lastWidth >= 0 && lastWidth != glyph->width) {
      proportionalReasons["glyphWidth"] = true;
    }
    if (lastXSpacing >= 0 && lastXSpacing != glyph->xSpacing) {
      proportionalReasons["xSpacing"] = true;
    }
    if (glyph->xStepBack != 0) {
      proportionalReasons["xStepBack"] = true;
    }

    // Check entry point
    if (glyph->fragmentsSameAsCode < 0) {
      if (nextEntryPoint >= mf::SmallGlyphEntry::EntryPoint::MAX) {
        largeFontReasons["entryPoint"] = true;
      }
      for (const auto &opr : glyph->operations) {
        nextEntryPoint += opr->codeLength;
      }
      while (nextEntryPoint % 2 != 0) {
        nextEntryPoint++;
      }
    }

    lastWidth = glyph->width;
    lastXSpacing = glyph->xSpacing;
  }

  bool largeFont = largeFontReasons.size() > 0;
  bool proportional = proportionalReasons.size() > 0;
  if (options.verbose) {
    if (largeFont) {
      std::cout << "  Normal Glyph Table Format applied due to:" << std::endl;
      for (const auto &reason : largeFontReasons) {
        std::cout << "    - " << reason.first << std::endl;
      }
    } else {
      std::cout << "  Small Glyph Table Format applied." << std::endl;
    }
    if (proportional) {
      std::cout << "  Proportional Glyph Table Format applied due to:"
                << std::endl;
      for (const auto &reason : proportionalReasons) {
        std::cout << "    - " << reason.first << std::endl;
      }
    } else {
      std::cout << "  Monospaced Glyph Table Format applied." << std::endl;
    }
  }

  // Construct bytecode block
  std::vector<uint8_t> bytecodes;
  for (const auto &glyphPair : glyphs) {
    const GlyphObject &glyph = glyphPair.second;
    if (glyph->fragmentsSameAsCode >= 0) {
      const GlyphObject &dupGlyph = glyphs[glyph->fragmentsSameAsCode];
      glyph->entryPoint = dupGlyph->entryPoint;
      glyph->byteCodeSize = dupGlyph->byteCodeSize;
      continue;
    }
    glyph->entryPoint = bytecodes.size();
    for (const auto &opr : glyph->operations) {
      opr->writeCodeTo(bytecodes);
    }
    if (!largeFont) {
      while (bytecodes.size() % 2 != 0) {
        bytecodes.push_back(mf::baseCodeOf(mf::Operator::ABO));
      }
    }
    glyph->byteCodeSize = bytecodes.size() - glyph->entryPoint;

    if (options.verbose && options.verboseForCode == glyph->code) {
      std::cout << "  Bytecode generated for glyph " << c2s(glyph->code)
                << ":" << std::endl
                << "    Entry Point: " << glyph->entryPoint << std::endl;
      glyph->report("    ");
    }
  }

  bool dummyFragmentInserted = false;
  while (fragTable.size() == 0 || fragTable.size() % 2 != 0) {
    fragTable.push_back(0x00);
    dummyFragmentInserted = true;
  }
  if (options.verbose && dummyFragmentInserted) {
    std::cout << "  Fragment Table size is odd or zero, adding a dummy entry."
              << std::endl;
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
    const GlyphObject &glyph = glyphPair.second;
    if (glyph->width > maxGlyphWidth) {
      maxGlyphWidth = glyph->width;
    }
  }

  uint8_t version = 1;

  uint8_t fontFlags = 0;
  fontFlags |= mf::FontFlags::VerticalFragment::place(options.verticalFrag);
  fontFlags |= mf::FontFlags::Msb1st::place(options.msb1st);
  fontFlags |= mf::FontFlags::LargeFont::place(largeFont);
  fontFlags |= mf::FontFlags::Proportional::place(proportional);

  blob.clear();

  // Font Header
  {
    uint8_t header[8] = {0};
    uint8_t *ptr = header;
    mf::FontHeader::FormatVersion::write(ptr, version, "formatVersion");
    mf::FontHeader::Flags::write(ptr, fontFlags, "fontFlags");
    mf::FontHeader::FirstCode::write(ptr, firstCode, "firstCode");
    mf::FontHeader::LastCode::write(ptr, lastCode, "lastCode");
    mf::FontHeader::FragmentTableSize::write(ptr, fragTable.size(),
                                             "fragmentTableSize");
    mf::FontHeader::MaxGlyphWidth::write(ptr, maxGlyphWidth, "maxGlyphWidth");
    mf::FontHeader::GlyphHeight::write(ptr, glyphHeight, "glyphHeight");
    mf::FontHeader::XMonoSpacing::write(ptr, 0, "xMonoSpacing");
    mf::FontHeader::YSpacing::write(ptr, ySpacing, "ySpacing");

    blob.insert(blob.end(), header, header + sizeof(header));
  }

  if (options.verbose) {
    std::cout << "  Font header generated:" << std::endl;
    dumpByteArray(blob, "    ");
#if 0
    mf::FontHeader header(blob.data());
    header.dumpHeader("    ");
#endif
  }

  // Glyph Table
  int glyphTableOffset = blob.size();
  int entrySize = 1;
  if (largeFont) entrySize *= 2;
  if (proportional) entrySize *= 2;
  if (options.verbose) {
    std::cout << "  Glyph entry size: " << entrySize << " bytes/glyph"
              << std::endl;
  }
  for (int code = firstCode; code <= lastCode; code++) {
    // Allocate space for the glyph entry
    const auto it = glyphs.find(code);
    bool missing = it == glyphs.end();
    bool empty = !missing && it->second->width == 0;
    if (missing || empty) {
      if (options.verbose) {
        if (missing) {
          std::cout << "  Glyph " << c2s(code) << " is missing."
                    << std::endl;
        } else if (empty) {
          std::cout << "  Glyph " << c2s(code) << " is empty, skipping."
                    << std::endl;
        }
      }
      for (int i = 0; i < entrySize; i++) {
        blob.push_back(0xFF);  // Fill with dummy data
      }
      continue;
    }

    const GlyphObject &glyph = it->second;
    uint8_t entryBuff[entrySize] = {0};
    uint8_t *ptr = entryBuff;

    if (largeFont) {
      mf::NormalGlyphEntry::EntryPoint::write(ptr, glyph->entryPoint,
                                              "entryPoint");
      ptr += mf::NormalGlyphEntry::SIZE;
    } else {
      mf::SmallGlyphEntry::EntryPoint::write(ptr, glyph->entryPoint,
                                             "entryPoint");
      ptr += mf::SmallGlyphEntry::SIZE;
    }

    if (largeFont) {
      mf::NormalGlyphDimension::GlyphWidth::write(ptr, glyph->width,
                                                  "glyphWidth");
      mf::NormalGlyphDimension::XSpacing::write(ptr, glyph->xSpacing,
                                                "xSpacing");
      mf::NormalGlyphDimension::XStepBack::write(ptr, glyph->xStepBack,
                                                 "xStepBack");
    } else {
      mf::SmallGlyphDimension::GlyphWidth::write(ptr, glyph->width,
                                                 "glyphWidth");
      mf::SmallGlyphDimension::XSpacing::write(ptr, glyph->xSpacing,
                                               "xSpacing");
      mf::SmallGlyphDimension::XStepBack::write(ptr, glyph->xStepBack,
                                                "xStepBack");
    }

    // append to blob
    blob.insert(blob.end(), entryBuff, entryBuff + entrySize);
  }

  if (blob.size() % 2 != 0) {
    blob.push_back(0xFF);
    if (options.verbose) {
      std::cout << "  Glyph table size is odd, adding a dummy byte."
                << std::endl;
    }
  }

  int glyphTableSize = blob.size() - glyphTableOffset;
  if (options.verbose) {
    std::cout << "  Glyph table generated, size: " << glyphTableSize << " bytes"
              << std::endl;
    dumpByteArray(blob, "    ", glyphTableOffset, glyphTableSize);
  }

  blob.insert(blob.end(), fragTable.begin(), fragTable.end());

  blob.insert(blob.end(), bytecodes.begin(), bytecodes.end());

  if (options.verbose) {
    std::cout << "  Blob size: " << blob.size() << " bytes" << std::endl;
  }
}

}  // namespace mamefont::mamec
