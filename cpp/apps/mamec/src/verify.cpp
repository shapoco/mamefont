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

  if (verbose) {
    std::cout << "  MameFont header:" << std::endl;
    mameFont.header.dumpHeader("    ");
  }

  uint16_t buffSize = mameFont.calcMaxGlyphBufferSize();
  if (verbose) {
    std::cout << "  Required buffer size: " << static_cast<int>(buffSize)
              << " Bytes" << std::endl;
  }
  if (buffSize < 0 || 1024 < buffSize) {
    throw std::runtime_error("Invalid buffer size");
  }
  std::vector<uint8_t> bufferVec(buffSize * 2);
  mf::Glyph mameGlyph;
  mameGlyph.data = bufferVec.data();

  for (int code = 0; code <= 255; code++) {
    mf::Debugger::setVerbose(code == verboseForCode);

    auto bmpGlyph = bmpFont->getGlyph(code);
    try {
      ret = mameFont.getGlyph(code, &mameGlyph);
      if (ret == mf::Status::SUCCESS) {
        ret = mamefont::decodeGlyph(mameFont, &mameGlyph);
      }
    } catch (const mf::MameFontException &e) {
      ret = e.status;
    }

    if (!bmpGlyph) {
      if (ret == mf::Status::GLYPH_NOT_DEFINED ||
          ret == mf::Status::CHAR_CODE_OUT_OF_RANGE) {
        // No glyph in MameFont, which is expected
      } else if (ret == mf::Status::SUCCESS) {
        std::cerr << "*ERROR: MameFont has a glyph for code " << c2s(code)
                  << ", but bitmap font does not." << std::endl;
        totalFailedGlyphs++;
      } else {
        std::cerr << "*ERROR: Getting glyph for code " << c2s(code) << ": "
                  << mf::statusToString(ret) << " (" << static_cast<int>(ret)
                  << ")" << std::endl;
        totalFailedGlyphs++;
      }
      continue;
    }

    if (ret != mf::Status::SUCCESS) {
      std::cerr << "*ERROR: Extracting glyph for code " << c2s(code) << ": "
                << mf::statusToString(ret) << " (" << static_cast<int>(ret)
                << ")" << std::endl;
      totalFailedGlyphs++;
      continue;
    }

    int numPixelDiff = 0;
    for (int y = 0; y < mameFont.fontHeight(); y++) {
      for (int x = 0; x < bmpGlyph->width; x++) {
        uint8_t expectedPixel =
            bmpGlyph->bmp->get(x, y, mameFont.fragFormat());
        uint8_t outputPixel = mameGlyph.getPixel(x, y - mameGlyph.yOffset);
        if (expectedPixel != outputPixel) {
          numPixelDiff++;
        }
        totalPixels++;
      }
    }
    if (numPixelDiff > 0) {
      std::cerr << "*ERROR: Glyph verification failed for code " << c2s(code)
                << ": " << numPixelDiff << " pixel differences." << std::endl;
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
