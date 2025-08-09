#include <iostream>

#include "mamec/verify.hpp"

namespace mamefont::mamec {

bool verifyGlyphs(const BitmapFont &bmpFont, const mf::Font &mameFont,
                  bool verbose, int verboseForCode) {
  mf::Status ret;

  if (verbose) {
    std::cout << "Verifying glyphs..." << std::endl;
  }

  int totalGlyphs = 0;
  int totalFailedGlyphs = 0;
  int totalPixels = 0;

  uint8_t stride;
  uint16_t buffSize = mameFont.getRequiredGlyphBufferSize(&stride);
  if (verbose) {
    std::cout << "  Required buffer size: " << static_cast<int>(buffSize)
              << " bytes, stride: " << static_cast<int>(stride) << std::endl;
  }
  std::vector<uint8_t> bufferVec(buffSize * 2);
  mf::GlyphBuffer glyphBuff(bufferVec.data(), stride);

  for (int code = 0; code <= 255; code++) {
    auto bmpGlyph = bmpFont->getGlyph(code);
    mf::Glyph mameGlyph;
    ret = mameFont.getGlyph(code, &mameGlyph);
    if (!bmpGlyph) {
      if (ret == mf::Status::GLYPH_NOT_DEFINED ||
          ret == mf::Status::CHAR_CODE_OUT_OF_RANGE) {
        // No glyph in MameFont, which is expected
      } else if (ret == mf::Status::SUCCESS) {
        std::cerr << "*ERROR: MameFont has a glyph for code " << code
                  << ", but bitmap font does not." << std::endl;
        totalFailedGlyphs++;
      } else {
        std::cerr << "*ERROR: Getting glyph for code " << code << ": "
                  << static_cast<int>(ret) << std::endl;
        totalFailedGlyphs++;
      }
      continue;
    }

    ret = mamefont::extractGlyph(mameFont, code, glyphBuff, nullptr,
                                 verboseForCode == code);
    if (ret != mf::Status::SUCCESS) {
      std::cerr << "*ERROR: Extracting glyph for code " << code << ": "
                << static_cast<int>(ret) << std::endl;
      totalFailedGlyphs++;
      continue;
    }

    int numPixelDiff = 0;
    for (int y = 0; y < mameFont.glyphHeight(); y++) {
      for (int x = 0; x < bmpGlyph->width; x++) {
        bool expectedPixel = bmpGlyph->bmp->get(x, y) >= 128;
        bool actualPixel = glyphBuff.getPixel(mameFont, mameGlyph, x, y);
        if (expectedPixel != actualPixel) {
          numPixelDiff++;
        }
        totalPixels++;
      }
    }
    if (numPixelDiff > 0) {
      std::cerr << "*ERROR: Glyph verification failed for code "
                << formatChar(code) << ": " << numPixelDiff
                << " pixel differences." << std::endl;
      totalFailedGlyphs++;
    }

    totalGlyphs++;
  }

  if (verbose) {
    std::cout << "  Totally " << totalGlyphs << " glyphs and " << totalPixels
              << " pixels checked." << std::endl;
    if (totalFailedGlyphs == 0) {
      std::cout << "  All glyphs verified successfully." << std::endl;
    }
  }

  if (totalFailedGlyphs > 0) {
    std::cerr << "*ERROR: " << totalFailedGlyphs
              << " glyphs failed verification." << std::endl;
  }

  return totalFailedGlyphs == 0;
}

}  // namespace mamefont::mamec
