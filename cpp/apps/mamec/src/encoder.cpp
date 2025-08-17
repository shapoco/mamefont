#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
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

  fontHeight = bmpFont->bodySize;
  ySpace = bmpFont->ySpace;
  pixelFormat = (bmpFont->bitsPerPixel == 1) ? PixelFormat::BW_1BIT
                                             : PixelFormat::GRAY_2BIT;

  int xSpaceMin = 99999;
  for (const auto &bmpGlyph : bmpFont->glyphs) {
    int xsp = bmpFont->defaultXSpacing - bmpGlyph->xAntiSpace;
    if (xsp < xSpaceMin) {
      xSpaceMin = xsp;
    }
  }
  xSpaceBase = xSpaceMin;

  std::map<std::string, bool> largeFormatReasons;
  int totalFrags = 0;
  for (const auto &bmpGlyph : bmpFont->glyphs) {
    if (!mf::SmallGlyphDim::GlyphWidth::inRange(bmpGlyph->width)) {
      largeFormatReasons["GlyphWidth"] = true;
      break;
    }
    int xsp = bmpFont->defaultXSpacing - bmpGlyph->xAntiSpace - xSpaceBase;
    if (!mf::SmallGlyphDim::XSpaceOffset::inRange(xsp)) {
      largeFormatReasons["XSpaceOffset"] = true;
      break;
    }
    if (!mf::SmallGlyphDim::XStepBack::inRange(bmpGlyph->xStepBack)) {
      largeFormatReasons["XStepBack"] = true;
      break;
    }
    totalFrags += bmpGlyph->bmp->width * bmpGlyph->bmp->height /
                  mf::getBitsPerPixel(pixelFormat);
  }
  if (totalFrags > 5000) {
    largeFormatReasons["Total Number of Fragments"] = true;
  }
  mayBeLargeFormat = !largeFormatReasons.empty();
  if (options.verbose) {
    if (mayBeLargeFormat) {
      std::cout << "  This may be large font due to: " << std::endl;
      for (const auto &reasonPair : largeFormatReasons) {
        std::cout << "    " << reasonPair.first << std::endl;
      }
    } else {
      std::cout << "  This may be small font." << std::endl;
    }
  }

  determineAltTopBottom(bmpFont);

  for (const auto &bmpGlyph : bmpFont->glyphs) {
    addGlyph(bmpFont, bmpGlyph);
  }
}

void Encoder::determineAltTopBottom(const BitmapFont &font) {
  if (!mayBeLargeFormat) {
    // altTop and altBottom are cannot be used on small fonts
    altTop = 0;
    altBottom = fontHeight;
  }

  std::vector<int> yEdgeCount;
  yEdgeCount.resize(fontHeight + 1, 0);
  for (const auto &bmpGlyph : font->glyphs) {
    int top, yMax;
    if (bmpGlyph->bmp->getEffectiveArea(pixelFormat, nullptr, nullptr, &top,
                                        &yMax)) {
      int bottom = yMax + 1;
      if (options.verticalFrag) {
        int ppf = mf::getPixelsPerFrag(pixelFormat);
        top = (top / ppf) * ppf;
        bottom = ((bottom + ppf - 1) / ppf) * ppf;
        if (bottom > fontHeight) bottom = fontHeight;
      }
      if (top >= fontHeight / 5) yEdgeCount[top] += 1;
      if (bottom < fontHeight) yEdgeCount[bottom] -= 1;
    }
  }

  int mostPositiveCount = 0;
  int mostPositiveY = 0;
  int mostNegativeCount = 0;
  int mostNegativeY = 0;
  for (int y = 0; y < yEdgeCount.size(); ++y) {
    if (yEdgeCount[y] >= mostPositiveCount) {
      mostPositiveCount = yEdgeCount[y];
      mostPositiveY = y;
    }
    if (yEdgeCount[y] <= mostNegativeCount) {
      mostNegativeCount = yEdgeCount[y];
      mostNegativeY = y;
    }
  }

  if (mostPositiveY < mostNegativeY) {
    altTop = mostPositiveY;
    altBottom = mostNegativeY;
  } else {
    altTop = 0;
    altBottom = fontHeight;
  }
}

void Encoder::addGlyph(const BitmapFont &bmpFont, const BitmapGlyph &bmpGlyph) {
  auto bmp = bmpGlyph->bmp;

  bool useAltTop = false;
  bool useAltBottom = false;
  if (mayBeLargeFormat) {
    int yMin, yMax;
    if (bmp->getEffectiveArea(pixelFormat, nullptr, nullptr, &yMin, &yMax)) {
      useAltTop = altTop <= yMin;
      useAltBottom = yMax < altBottom;
    } else {
      useAltTop = true;
      useAltBottom = true;
    }
  }

  if (useAltTop || useAltBottom) {
    int top = useAltTop ? altTop : 0;
    int bottom = useAltBottom ? altBottom : fontHeight;
    bmp = bmp->crop(0, top, bmp->width, bottom - top);
    if (options.verbose && options.verboseForCode == bmpGlyph->code) {
      std::cout << "  Alternative Top/Bottom used for: " << c2s(bmpGlyph->code)
                << " top=" << (useAltTop ? 1 : 0) << ", "
                << " bottom=" << (useAltBottom ? 1 : 0) << std::endl;
    }
  }

  if (options.verbose && options.verboseForCode == bmpGlyph->code) {
    std::cout << "  glyphWidth=" << bmp->width << ", "
              << "glyphHeight=" << bmp->height << std::endl;
  }

  auto frags = bmp->toFragments(options.verticalFrag, options.farPixelFirst,
                                pixelFormat);

  std::vector<frag_t> compareMask;
  int numFrags = frags.size();
  compareMask.resize(numFrags, 0xFF);
  if (!options.forceZeroPadding) {
    int w = bmp->width;
    int h = bmp->height;
    int viewPort = options.verticalFrag ? h : w;
    int trackLength = options.verticalFrag ? w : h;
    int bpp = mf::getBitsPerPixel(pixelFormat);
    int ppf = mf::getPixelsPerFrag(pixelFormat);
    int numTracks = (viewPort + (ppf - 1)) / ppf;
    int maskWidth = (viewPort - (numTracks - 1) * ppf) * bpp;
    frag_t lastTrackMask = (1 << maskWidth) - 1;
    if (options.farPixelFirst) {
      lastTrackMask <<= (8 - maskWidth);
    }
    for (int i = numFrags - trackLength; i < numFrags; i++) {
      compareMask[i] = lastTrackMask;
    }
  }

  int xspo = bmpFont->defaultXSpacing - bmpGlyph->xAntiSpace - xSpaceBase;
  glyphs[bmpGlyph->code] = std::make_shared<GlyphObjectClass>(
      bmpGlyph->code, frags, compareMask, bmp->width, bmp->height,
      options.verticalFrag, options.farPixelFirst, xspo, bmpGlyph->xStepBack,
      useAltTop, useAltBottom);
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
    std::cout << "Detecting fragment duplications..." << std::endl;
  }
  detectFragmentDuplications("  ");

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

void Encoder::detectFragmentDuplications(std::string indent) {
  std::map<int, std::map<int, bool>> dupTree;

  // Find a glyph that is completely contained within the beginning of another
  // glyph's fragment
  for (auto &thisPair : glyphs) {
    GlyphObject &thisGlyph = thisPair.second;
    const VecRef thisFrags(thisGlyph->fragments, pixelFormat);
    const VecRef thisMask(thisGlyph->compareMask, pixelFormat);
    int thisSize = thisFrags.size;
    int thisCode = thisGlyph->code;

    int bestDupSrcCode = -1;
    size_t bestDupSrcSize = 0;
    for (auto &otherPair : glyphs) {
      GlyphObject &otherGlyph = otherPair.second;
      int otherSize = otherGlyph->fragments.size();
      int otherCode = otherGlyph->code;

      if (otherCode == thisCode) continue;
      if (otherSize < thisSize) continue;
      if (otherSize == thisSize && otherCode > thisCode) continue;

      const VecRef otherFrags(otherGlyph->fragments, 0, thisSize, pixelFormat);
      const VecRef otherMask(otherGlyph->compareMask, 0, thisSize, pixelFormat);

      if (!maskedEqual(thisFrags, otherFrags, thisMask)) {
        continue;
      }
      if (!maskedEqual(thisMask, otherMask, thisMask)) {
        continue;
      }

      if (otherSize > bestDupSrcSize) {
        bestDupSrcSize = otherSize;
        bestDupSrcCode = otherGlyph->code;
      }
    }

    if (bestDupSrcCode >= 0) {
      thisGlyph->fragDupSrcCode = bestDupSrcCode;
      dupTree[bestDupSrcCode][thisCode] = true;
    }
  }

  // Resolve chained references
  for (auto &rootPair : glyphs) {
    GlyphObject &rootGlyph = rootPair.second;
    if (rootGlyph->fragDupSrcCode >= 0) continue;

    std::stack<GlyphObject> stack;
    stack.push(rootGlyph);

    while (!stack.empty()) {
      GlyphObject &parentGlyph = stack.top();
      stack.pop();

      if (parentGlyph->fragDupSrcCode >= 0) {
        parentGlyph->fragDupSrcCode = rootGlyph->code;
        if (options.verbose) {
          std::cout << indent
                    << "Fragment duplication found: " << c2s(parentGlyph->code)
                    << " --> " << c2s(rootGlyph->code) << std::endl;
        }
      }

      if (dupTree.contains(rootGlyph->code)) {
        for (auto &childPair : dupTree[parentGlyph->code]) {
          auto &childGlyph = glyphs[childPair.first];
          stack.push(childGlyph);
        }
      }
    }
  }

  for (auto &thisPair : glyphs) {
    GlyphObject &thisGlyph = thisPair.second;
    if (thisGlyph->fragDupSrcCode >= 0) {
      glyphs[thisGlyph->fragDupSrcCode]
          ->barrierPosForSolveFragDup[thisGlyph->fragments.size()] = true;
    }
  }
}

void Encoder::generateInitialOperations(GlyphObject &glyph, bool verbose,
                                        std::string indent) {
  int numFrags = glyph->fragments.size();

  if (verbose) {
    std::cout << indent << "Fragments:" << std::endl;
    dumpByteArray(glyph->fragments, indent + "  ");
    std::cout << indent << "Compare Mask:" << std::endl;
    dumpByteArray(glyph->compareMask, indent + "  ");
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

    VecRef future(glyph->fragments, curr->pos, numFrags, pixelFormat);
    VecRef mask(glyph->compareMask, curr->pos, numFrags, pixelFormat);

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

      // Reject operation that cross barriers for fragment duplication
      bool barrierViolated = false;
      for (auto &barrierPair : glyph->barrierPosForSolveFragDup) {
        int barrierPos = barrierPair.first;
        int outputStart = curr->pos;
        int outputEnd = outputStart + opr->output.size();
        if (outputStart < barrierPos && barrierPos < outputEnd) {
          barrierViolated = true;
          break;
        }
        opr->afterBarrier |= (outputStart == barrierPos);
        opr->beforeBarrier |= (outputEnd == barrierPos);
      }
      if (barrierViolated) continue;

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
    throw std::runtime_error("No solutions found for " + c2s(glyph->code));
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

  int stateWidth;
  switch (pixelFormat) {
    case mf::PixelFormat::BW_1BIT:
      stateWidth = 8;
      break;
    case mf::PixelFormat::GRAY_2BIT:
      stateWidth = 12;
      break;
    default:
      throw std::invalid_argument("Unsupported pixel format");
  }

  uint16_t shiftMask = (1 << stateWidth) - 1;

  uint16_t modifier = (1 << size) - 1;
  if (right) modifier <<= (stateWidth - size);
  if (!postSet) modifier = ~modifier;

  frag_t workFrag = ctx.state->lastFrag;
  uint16_t state;
  switch (pixelFormat) {
    case mf::PixelFormat::BW_1BIT:
      state = workFrag;
      break;
    case mf::PixelFormat::GRAY_2BIT:
      state = Encoder::encodeShiftState2bpp(workFrag, right, postSet);
      break;
    default:
      throw std::invalid_argument("Unsupported pixel format");
  }
  bool changeDetected = false;

  int offset = 0;
  for (int rpt = (preShift ? 0 : 1); rpt <= rptMax; rpt++) {
    int startPhase = (rpt == 0 && preShift) ? (period - 1) : 0;
    for (int phase = startPhase; phase < period; phase++) {
      frag_t lastFrag = workFrag;
      if (phase == period - 1) {
        if (right) {
          state = (state >> size) & shiftMask;
        } else {
          state = (state << size) & shiftMask;
        }
        if (postSet) {
          state |= modifier;
        } else {
          state &= modifier;
        }
      }
      switch (pixelFormat) {
        case mf::PixelFormat::BW_1BIT:
          workFrag = state;
          break;
        case mf::PixelFormat::GRAY_2BIT:
          workFrag = Encoder::decodeShiftState2bpp(state);
          break;
        default:
          throw std::invalid_argument("Unsupported pixel format");
      }

      if (!maskedEqual(lastFrag, workFrag, ctx.compareMask[offset])) {
        changeDetected = true;
      }

      if (maskedEqual(workFrag, ctx.future[offset], ctx.compareMask[offset])) {
        output.push_back(workFrag);
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
      VecRef pastRef(pastBuff, iPastFrom, iPastTo, pixelFormat);
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

      VecRef pastRef(pastBuff, iPastFrom, iPastTo, pixelFormat);

      for (bool byteReverse : {false, true}) {
        for (bool pixelReverse : {false, true}) {
          for (bool inverse : {false, true}) {
            uint8_t cpxFlags = 0;
            cpxFlags |= mf::CPX::ByteReverse::place(byteReverse);
            cpxFlags |= mf::CPX::PixelReverse::place(pixelReverse);
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
          glyph->replaceOperation(i, makeLUP(newIndex, opr->output[0]));
        } else {
          glyph->replaceOperation(i, makeLDI(opr->output[0], 0));
          std::cerr << "  *WARNING: LUP unexpectedly replaced with LDI for "
                    << c2s(glyph->code) << ", since fragment 0x"
                    << u2x8(opr->output[0]) << " not found in fragment table."
                    << std::endl;
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
          glyph->replaceOperation(i, makeLUP(index, frag));
        }
      }
    }

    // replace pair of LUP with LUD if possible
    Operation opr1 = nullptr;
    int idx1 = -1;
    frag_t frag1 = 0x00;
    for (int i = 0; i < glyph->operations.size(); i++) {
      auto &opr = glyph->operations[i];
      Operation opr2 = nullptr;
      int idx2 = -1;
      frag_t frag2 = 0x00;
      if (opr->op == mf::Operator::LUP) {
        opr2 = opr;
        idx2 = mf::LUP::Index::read(opr2->code[0]);
        frag2 = opr2->output[0];
      }
      if (idx1 >= 0 && idx2 >= 0) {
        int step = idx2 - idx1;
        if (idx1 <= mf::LUD::Index::MAX && (step == 0 || step == 1)) {
          if (!opr1->beforeBarrier && !opr2->afterBarrier) {
            auto lud = makeLUD(idx1, step, frag1, frag2);
            glyph->replaceOperation(i - 1, lud);
            glyph->operations.erase(glyph->operations.begin() + i);
            lud->beforeBarrier = opr2->beforeBarrier;
            i--;
            opr2 = nullptr;
            idx2 = -1;
            frag2 = 0x00;
            numReplacedOps++;
          }
        }
      }
      opr1 = opr2;
      idx1 = idx2;
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

  // Check if fragment duplications are solvable
  for (const auto &thisPair : glyphs) {
    auto &thisGlyph = thisPair.second;
    if (thisGlyph->fragDupSrcCode < 0) continue;

    int thisSize = thisGlyph->fragments.size();
    int thisWidth = thisGlyph->width;

    auto otherGlyph = glyphs[thisGlyph->fragDupSrcCode];
    int otherSize = 0;
    int otherWidth = otherGlyph->width;
    bool success = false;
    Operation violator = nullptr;
    for (const auto &opr : otherGlyph->operations) {
      int newSize = otherSize + opr->output.size();
      if (newSize >= thisSize) {
        if (newSize == thisSize) {
          success = true;
        } else {
          violator = opr;
        }
        break;
      }
      otherSize = newSize;
    }
    if (!success) {
      std::cerr << "*WARNING: Failed to solve fragment duplication: "
                << c2s(thisGlyph->code) << " --> " << c2s(otherGlyph->code)
                << " because barrier crossed by operation "
                << mnemonicOf(violator->op) << std::endl;
      thisGlyph->fragDupSrcCode = -1;  // Mark as unresolved
    }
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
    if (!mf::SmallGlyphDim::GlyphWidth::inRange(glyph->width)) {
      largeFontReasons["glyphWidth"] = true;
    }
    if (!mf::SmallGlyphDim::XSpaceOffset::inRange(glyph->xSpaceOffset)) {
      largeFontReasons["xSpaceOffset"] = true;
    }
    if (!mf::SmallGlyphDim::XStepBack::inRange(glyph->xStepBack)) {
      largeFontReasons["xStepBack"] = true;
    }
    if (lastWidth >= 0 && lastWidth != glyph->width) {
      proportionalReasons["glyphWidth"] = true;
    }
    if (lastXSpacing >= 0 && lastXSpacing != glyph->xSpaceOffset) {
      proportionalReasons["xSpace"] = true;
    }
    if (glyph->xStepBack != 0) {
      proportionalReasons["xStepBack"] = true;
    }

    // Check entry point
    if (glyph->fragDupSrcCode < 0) {
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
    if (glyph->useAltTop || glyph->useAltBottom) {
      largeFontReasons["altTop/altBottom"] = true;
    }

    lastWidth = glyph->width;
    lastXSpacing = glyph->xSpaceOffset;
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

  std::vector<int> codes;
  for (const auto &glyphPair : glyphs) {
    codes.push_back(glyphPair.first);
  }
  std::sort(codes.begin(), codes.end());
  int firstCode = codes.front();
  int lastCode = codes.back();

  // Construct bytecode block and fix entry points
  std::vector<uint8_t> bytecodes;
  for (int code : codes) {
    const GlyphObject &glyph = glyphs[code];
    if (glyph->fragDupSrcCode >= 0) {
      continue;
    }
    glyph->entryPoint = bytecodes.size();
    for (const auto &opr : glyph->operations) {
      opr->writeCodeTo(bytecodes);
    }
    if (!largeFont && code != lastCode) {
      while (bytecodes.size() % 2 != 0) {
        bytecodes.push_back(mf::baseCodeOf(mf::Operator::ABO));
      }
    }
    glyph->byteCodeSize = bytecodes.size() - glyph->entryPoint;

    if (options.verbose && options.verboseForCode == glyph->code) {
      std::cout << "  Bytecode generated for glyph " << c2s(glyph->code) << ":"
                << std::endl
                << "    Entry Point: " << glyph->entryPoint << std::endl;
      glyph->report("    ");
    }
  }

  for (int i = 0; i < 3; i++) {
    bytecodes.push_back(mf::baseCodeOf(mf::Operator::ABO));
  }

  // Solve fragment duplications
  for (const auto &glyphPair : glyphs) {
    const GlyphObject &thisGlyph = glyphPair.second;
    if (thisGlyph->fragDupSrcCode < 0) {
      continue;
    }
    GlyphObject &otherGlyph = glyphs[thisGlyph->fragDupSrcCode];
    while (otherGlyph->fragDupSrcCode >= 0) {
      otherGlyph = glyphs[otherGlyph->fragDupSrcCode];
    }
    if (otherGlyph->entryPoint < 0 ||
        otherGlyph->byteCodeSize == mf::DUMMY_ENTRY_POINT) {
      throw std::runtime_error(
          "Fragment duplication cannot solved "
          "due to source glyph does not have valid entry point: " +
          c2s(thisGlyph->code) + " --> " + c2s(otherGlyph->code));
    }

    if (options.verbose) {
      int thisSize = thisGlyph->fragments.size();
      int thisWidth = thisGlyph->width;
      int otherSize = otherGlyph->fragments.size();
      int otherWidth = otherGlyph->width;
      bool complete = (otherSize == thisSize) && (otherWidth == thisWidth);
      std::cout << "  Fragment duplication solved ("
                << (complete ? "complete dup." : "partial dup.")
                << "): " << c2s(thisGlyph->code) << " --> "
                << c2s(otherGlyph->code) << std::endl;
    }
    thisGlyph->entryPoint = otherGlyph->entryPoint;
    thisGlyph->byteCodeSize = otherGlyph->byteCodeSize;
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

  int maxGlyphWidth = 1;
  for (const auto &glyphPair : glyphs) {
    const GlyphObject &glyph = glyphPair.second;
    if (glyph->width > maxGlyphWidth) {
      maxGlyphWidth = glyph->width;
    }
  }

  uint8_t version = 0x00;  // todo: 0x01 or 0x10

  uint8_t fontFlags = 0;
  fontFlags |= mf::FontFlags::VerticalFragment::place(options.verticalFrag);
  fontFlags |= mf::FontFlags::FarPixelFirst::place(options.farPixelFirst);
  fontFlags |= mf::FontFlags::LargeFont::place(largeFont);
  fontFlags |= mf::FontFlags::Proportional::place(proportional);
  fontFlags |=
      mf::FontFlags::FragFormat::place(static_cast<uint8_t>(pixelFormat));

  blob.clear();

  // Font Header
  {
    uint8_t header[mf::FontHeader::SIZE] = {0};
    uint8_t *ptr = header;
    mf::FontHeader::FormatVersion::write(ptr, version, "formatVersion");
    mf::FontHeader::Flags::write(ptr, fontFlags, "fontFlags");
    mf::FontHeader::FirstCode::write(ptr, firstCode, "firstCode");
    mf::FontHeader::LastCode::write(ptr, lastCode, "lastCode");
    mf::FontHeader::MaxGlyphWidth::write(ptr, maxGlyphWidth, "maxGlyphWidth");
    mf::FontHeader::GlyphHeight::write(ptr, fontHeight, "fontHeight");
    mf::FontHeader::XSpace::write(ptr, xSpaceBase, "xSpace");
    mf::FontHeader::YSpace::write(ptr, ySpace, "ySpace");
    mf::FontHeader::AltTop::write(ptr, altTop, "altTop");
    mf::FontHeader::AltBottom::write(ptr, altBottom, "altBottom");
    mf::FontHeader::FragmentTableSize::write(ptr, fragTable.size(),
                                             "fragmentTableSize");

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
          std::cout << "  Glyph " << c2s(code) << " is missing." << std::endl;
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
      mf::NormalGlyphEntry::UseAltTop::write(ptr, glyph->useAltTop);
      mf::NormalGlyphEntry::UseAltBottom::write(ptr, glyph->useAltBottom);
      ptr += mf::NormalGlyphEntry::SIZE;
    } else {
      mf::SmallGlyphEntry::EntryPoint::write(ptr, glyph->entryPoint,
                                             "entryPoint");
      ptr += mf::SmallGlyphEntry::SIZE;
    }

    if (largeFont) {
      mf::NormalGlyphDim::GlyphWidth::write(ptr, glyph->width, "glyphWidth");
      mf::NormalGlyphDim::XSpaceOffset::write(ptr, glyph->xSpaceOffset,
                                              "xSpaceOffset");
      mf::NormalGlyphDim::XStepBack::write(ptr, glyph->xStepBack, "xStepBack");
    } else {
      mf::SmallGlyphDim::GlyphWidth::write(ptr, glyph->width, "glyphWidth");
      mf::SmallGlyphDim::XSpaceOffset::write(ptr, glyph->xSpaceOffset,
                                             "xSpaceOffset");
      mf::SmallGlyphDim::XStepBack::write(ptr, glyph->xStepBack, "xStepBack");
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

void Encoder::checkFragmentDuplicationSolvable() {}

}  // namespace mamefont::mamec
