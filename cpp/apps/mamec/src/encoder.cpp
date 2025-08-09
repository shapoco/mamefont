#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "mamec/bitmap_glyph.hpp"
#include "mamec/encoder.hpp"
#include "mamec/generation_state.hpp"
#include "mamec/gray_bitmap.hpp"
#include "mamec/mame_glyph.hpp"

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

void Encoder::generateInitialOperations(MameGlyph &glyph) {
  if (options.verbose && options.verboseForCode == glyph->code) {
    std::cout << "    Generating operations for " << formatChar(glyph->code)
              << std::endl;
  }

  int numFrags = glyph->fragments.size();

  auto compareMask = glyph->createCompareMaskArray();

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

  std::map<size_t, GenerationState> waitings;
  auto first = std::make_shared<GenerationStateClass>(0, 0, nullptr);
  first->bestCost = 0;
  waitings[first->id] = first;

  std::vector<GenerationState> goalStates;
  while (!waitings.empty()) {
    GenerationState curr = nullptr;
    int bestCost = 999999;
    for (auto it = waitings.begin(); it != waitings.end(); ++it) {
      if (it->second->bestCost < bestCost) {
        curr = it->second;
        bestCost = curr->bestCost;
      }
    }
    waitings.erase(curr->id);

    if (curr->pos >= numFrags) {
      goalStates.push_back(curr);
      continue;
    }

    auto future = VecRef(glyph->fragments, curr->pos, numFrags);
    auto mask = VecRef(compareMask, curr->pos, numFrags);

    std::vector<Operation> oprs;
    tryLDI(oprs, curr, future, mask);
    tryRPT(oprs, curr, future, mask);

    for (const auto &opr : oprs) {
      GenerationState next = curr;
      for (fragment_t frag : opr->generated) {
        if (next->nextState.find(frag) == next->nextState.end()) {
          auto newState =
              std::make_shared<GenerationStateClass>(next->pos + 1, frag, next);
          next->nextState[frag] = newState;
          next = newState;
          waitings[next->id] = next;
        } else {
          next = next->nextState[frag];
        }
      }

      int nextCost = curr->bestCost + opr->cost;
      if (nextCost < next->bestCost) {
        next->bestCost = nextCost;
        next->bestOpr = opr;
        next->bestPrev = curr;
        waitings[next->id] = next;
      }
    }
  }

  if (goalStates.empty()) {
    throw std::runtime_error("No solutions found for " +
                             formatChar(glyph->code));
  }

  GenerationState bestGoal = nullptr;
  int bestCost = 999999;
  for (const auto &goal : goalStates) {
    if (goal->bestCost < bestCost) {
      bestGoal = goal;
      bestCost = goal->bestCost;
    }
  }
  GenerationState p = bestGoal;
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

void Encoder::tryLDI(std::vector<Operation> &oprs, GenerationState &state,
                     const VecRef &future, const VecRef &mask) {
  oprs.push_back(makeLDI(future[0]));
}

void Encoder::tryRPT(std::vector<Operation> &oprs, GenerationState &state,
                     const VecRef &future, const VecRef &mask) {
  int rptMax = std::min((size_t)mf::RPT_REPEAT_COUNT::MAX, future.size);
  int rptStep = mf::RPT_REPEAT_COUNT::STEP;
  for (int rpt = 1; rpt <= rptMax; rpt += rptStep) {
    if (future[rpt - 1] != state->lastFrag) {
      break;
    }
    if (rpt >= mf::RPT_REPEAT_COUNT::MIN) {
      oprs.push_back(makeRPT(future[0], rpt));
    }
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
