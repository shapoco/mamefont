#include <iostream>

#include <nlohmann/json.hpp>

#include "mamec/bitmap_font.hpp"
#include "mamec/encoder.hpp"
#include "mamec/file_type_bmp.hpp"
#include "mamec/gray_bitmap.hpp"
#include "mamec/mamec_common.hpp"
#include "mamec/verify.hpp"

namespace mamefont::mamec {

std::string importBitmapFont(const BitmapFont bmpFont,
                             std::vector<uint8_t>& blob,
                             const EncodeOptions& options) {
  std::string fontName = bmpFont->fullName;

  if (options.verbose) {
    std::cout << "  Font family       : " << bmpFont->familyName.c_str()
              << std::endl;
    std::cout << "  Body size         : " << bmpFont->bodySize << std::endl;
    std::cout << "  Cap height        : " << bmpFont->capHeight << std::endl;
    std::cout << "  Ascender spacing  : " << bmpFont->ascenderSpacing
              << std::endl;
    std::cout << "  Weight            : " << bmpFont->weight << std::endl;
    std::cout << "  Default X spacing : " << bmpFont->defaultXSpacing
              << std::endl;
    std::cout << "  Y spacing         : " << bmpFont->ySpace << std::endl;
    std::cout << "  Bits per pixel    : " << bmpFont->bitsPerPixel << std::endl;
  }

  Encoder encoder(options);
  encoder.addFont(bmpFont);
  encoder.encode();
  encoder.generateBlob();

  blob = encoder.blob;

  return fontName;
}
}  // namespace mamefont::mamec