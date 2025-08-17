#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <cmath>

#include <nlohmann/json.hpp>

#include "mamec/bitmap_font.hpp"
#include "mamec/bitmap_glyph.hpp"
#include "mamec/gray_bitmap.hpp"
#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

namespace fs = std::filesystem;

BitmapFontClass::BitmapFontClass(const std::string &bmpPathStr) {
  fs::path bmpPath = fs::path(bmpPathStr);
  std::string jsonPathStr = bmpPath.replace_extension(".json").string();

  std::vector<int> codes;

  // parse family name and dimensions from directory name
  {
    fullName = bmpPath.parent_path().filename().string();
    size_t lastUnderlinePos = fullName.find_last_of('_');
    if (lastUnderlinePos == std::string::npos) {
      throw std::runtime_error(
          "Invalid directory name format, expected "
          "'FamilyName_DimensionIdentifier': '" +
          fullName + "'");
    }

    familyName = fullName.substr(0, lastUnderlinePos);

    auto dims = parseDimensionIdentifier(fullName, lastUnderlinePos + 1);

    if (dims.find(Dimension::BODY_SIZE) != dims.end()) {
      bodySize = dims[Dimension::BODY_SIZE];
    }

    if (dims.find(Dimension::CAP_HEIGHT) != dims.end()) {
      capHeight = dims[Dimension::CAP_HEIGHT];
    }

    if (dims.find(Dimension::ASCENDER_SPACING) != dims.end()) {
      ascenderSpacing = dims[Dimension::ASCENDER_SPACING];
    }

    if (dims.find(Dimension::WEIGHT) != dims.end()) {
      weight = dims[Dimension::WEIGHT];
    }

    if (dims.find(Dimension::BITS_PER_PIXEL) != dims.end()) {
      bitsPerPixel = dims[Dimension::BITS_PER_PIXEL];
    }
  }

  // parse JSON file
  {
    std::ifstream jsonFile(jsonPathStr);
    if (!jsonFile) {
      throw std::runtime_error("JSON file not found: " + jsonPathStr);
    }
    auto jsonObj = nlohmann::json::parse(jsonFile, nullptr, false);
    jsonFile.close();

    if (jsonObj.is_discarded()) {
      throw std::runtime_error("Invalid format");
    }

    if (jsonObj.contains("codes")) {
      for (const auto &codeRange : jsonObj.at("codes")) {
        int codeFrom = -1, codeTo = -1;
        codeRange.at("from").get_to(codeFrom);
        codeRange.at("to").get_to(codeTo);
        if (codeFrom < 0 || codeFrom > 255 || codeTo < 0 || codeTo > 255 ||
            codeFrom > codeTo) {
          throw std::runtime_error("Invalid code range");
        }
        for (int code = codeFrom; code <= codeTo; ++code) {
          codes.push_back(code);
        }
      }
    }

    if (jsonObj.contains("bits_per_pixel")) {
      jsonObj.at("bits_per_pixel").get_to(bitsPerPixel);
    }

    if (jsonObj.contains("dimensions")) {
      const auto &dimsObj = jsonObj.at("dimensions");

      if (dimsObj.contains("body_size")) {
        dimsObj.at("body_size").get_to(bodySize);
      }

      if (dimsObj.contains("x_spacing")) {
        dimsObj.at("x_spacing").get_to(defaultXSpacing);
      }

      if (dimsObj.contains("y_spacing")) {
        dimsObj.at("y_spacing").get_to(ySpace);
      }
    }
  }

  if (capHeight == DIMENSION_INVALID) capHeight = bodySize;
  if (weight == DIMENSION_INVALID) weight = 1;
  if (ascenderSpacing == DIMENSION_INVALID) ascenderSpacing = 0;

  if (defaultXSpacing == DIMENSION_INVALID) {
    defaultXSpacing = std::ceil(bodySize / 16.0);
  }

  if (ySpace == DIMENSION_INVALID) {
    ySpace = std::ceil((bodySize - ascenderSpacing) * 1.2 - bodySize);
  }

  if (bitsPerPixel == DIMENSION_INVALID) {
    bitsPerPixel = 1;
  }

  // extract glyphs
  {
    auto bmp = std::make_shared<GrayBitmapClass>(bmpPathStr);

    int glyph_index = 0;
    int y = bodySize;

    while (y < bmp->height) {
      bool found = false;
      int x = 0;
      while (x < bmp->width) {
        if (bmp->get(x, y) == GrayBitmapClass::MARKER_GLYPH) {
          auto glyph = decodeGlyph(codes[glyph_index], bmp, x, y);
          glyph_index += 1;
          glyphs.push_back(glyph);
          x += glyph->width;
          found = true;
        } else {
          x += 1;  // Move to next pixel
        }
      }

      if (found) {
        y += 1 + bodySize;
      } else {
        y += 1;
      }
    }
  }
}

const BitmapGlyph BitmapFontClass::getGlyph(int code) const {
  for (const auto &glyph : glyphs) {
    if (glyph->code == code) {
      return glyph;
    }
  }
  return nullptr;
}

std::shared_ptr<BitmapGlyphClass> BitmapFontClass::decodeGlyph(
    int code, const std::shared_ptr<GrayBitmapClass> &bmp, int x, int y) {
  // Find end of glyph marker
  int w = 0;
  while (bmp->get(x + w, y, 0) == GrayBitmapClass::MARKER_GLYPH) {
    w += 1;
  }
  if (w <= 0) {
    throw std::runtime_error("Glyph marker not found");
  }
  int glyphWidth = w;

  // Find left side anti spacer
  w = 0;
  while (bmp->get(x + w, y + 1, 0) == GrayBitmapClass::MARKER_SPACING) {
    w += 1;
  }
  int xStepBack = w;

  // Find right side anti spacer
  int xAntiSpace = 0;
  if (xStepBack < glyphWidth) {
    w = 0;
    int r = glyphWidth - 1;
    while (bmp->get(x + r - w, y + 1, 0) == GrayBitmapClass::MARKER_SPACING) {
      w += 1;
    }
    xAntiSpace = w;
  } else {
    xAntiSpace = 0;
  }

  // Extract Glyph Bitmap
  int fontHeight = bodySize;
  auto cropped = bmp->crop(x, y - fontHeight, glyphWidth, fontHeight);

  return std::make_shared<BitmapGlyphClass>(
      code, glyphWidth, fontHeight, cropped, xStepBack, xAntiSpace);
}

std::unordered_map<Dimension, int> parseDimensionIdentifier(
    const std::string &s, size_t startPos) {
  std::unordered_map<Dimension, int> dims;
  size_t len = s.length();
  size_t pos = startPos;
  try {
    while (pos < len) {
      char dimChar = s[pos++];

      Dimension dim;
      switch (dimChar) {
        case 's':
          dim = Dimension::BODY_SIZE;
          break;
        case 'c':
          dim = Dimension::CAP_HEIGHT;
          break;
        case 'a':
          dim = Dimension::ASCENDER_SPACING;
          break;
        case 'w':
          dim = Dimension::WEIGHT;
          break;
        case 'g':
          dim = Dimension::BITS_PER_PIXEL;
          break;
        default:
          throw std::runtime_error("Unknown dimension identifier: '" +
                                   std::string(1, dimChar) + "'");
      }

      if (dims.find(dim) != dims.end()) {
        std::cerr << "*Warning: Duplicate dimension identifier '" << dimChar
                  << "'" << std::endl;
      }

      if (pos >= len) {
        throw std::runtime_error("Number expected.");
      }

      int sign = 1;
      if (s[pos] == '-') {
        sign = -1;
        pos++;
      }

      if (pos >= len || s[pos] < '0' || s[pos] > '9') {
        throw std::runtime_error("One or more digits expected.");
      }

      int number = 0;
      while (pos < len && '0' <= s[pos] && s[pos] <= '9') {
        number = number * 10 + (s[pos++] - '0');
      }
      number *= sign;

      dims[dim] = number;
    }
  } catch (const std::exception &e) {
    throw std::runtime_error("Dimension syntax error at position " +
                             std::to_string(pos) + ": " + e.what());
  }
  return dims;
}

}  // namespace mamefont::mamec
