#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>

#include "mamec/mamec.hpp"

using namespace mamefont::mamec;

static constexpr char OPT_INPUT = 'i';
static constexpr char OPT_OUTPUT = 'o';
static constexpr char OPT_ENCODING = 'e';
static constexpr char OPT_NO_CPX = 0x82;
static constexpr char OPT_NO_SFI = 0x83;
static constexpr char OPT_FORCE_ZERO_PADDING = 0x84;
static constexpr char OPT_VERBOSE = 0x85;

static struct option long_opts[] = {
    {"input", required_argument, 0, OPT_INPUT},
    {"output", required_argument, 0, OPT_OUTPUT},
    {"encoding", required_argument, 0, OPT_ENCODING},
    {"no_cpx", no_argument, 0, OPT_NO_CPX},
    {"no_sfi", no_argument, 0, OPT_NO_SFI},
    {"force_zero_padding", no_argument, 0, OPT_FORCE_ZERO_PADDING},
    {"verbose", optional_argument, 0, OPT_VERBOSE},
    {0, 0, 0, 0},
};

int main(int argc, char *argv[]) {
  std::string argInput;
  std::string argOutput;
  std::string argEncoding("HL");
  bool argNoCPX = false;
  bool argNoSFI = false;
  bool argForceZeroPadding = false;
  bool argVerbose = false;
  std::string argVerboseForCodeStr;
  int argVerboseForCode = -1;

  char short_opts[256];
  snprintf(short_opts, sizeof(short_opts), "%c:%c:%c:", OPT_INPUT, OPT_OUTPUT,
           OPT_ENCODING);

  int opt;
  while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (opt) {
      case OPT_INPUT:
        argInput = optarg;
        break;
      case OPT_OUTPUT:
        argOutput = optarg;
        break;
      case OPT_ENCODING:
        argEncoding = optarg;
        break;
      case OPT_NO_CPX:
        argNoCPX = true;
        break;
      case OPT_NO_SFI:
        argNoSFI = true;
        break;
      case OPT_FORCE_ZERO_PADDING:
        argForceZeroPadding = true;
        break;
      case OPT_VERBOSE:
        argVerbose = true;
        argVerboseForCodeStr = optarg ? optarg : "";
        break;
      case '?':
        return 1;
    }
  }

  if (argVerbose && !argVerboseForCodeStr.empty()) {
    if (argVerboseForCodeStr.find("0x") == 0) {
      // 0x で始まる場合
      try {
        argVerboseForCode = std::stoi(argVerboseForCodeStr, nullptr, 16);
      } catch (const std::exception &e) {
        std::cerr << "*WARNING: Invalid verbose code: " << argVerboseForCodeStr
                  << std::endl;
      }
    } else {
      // それ以外は1文字として解釈
      if (argVerboseForCodeStr.length() == 1) {
        argVerboseForCode = argVerboseForCodeStr[0];
      } else {
        std::cerr << "*WARNING: Invalid verbose code: " << argVerboseForCodeStr
                  << std::endl;
      }
    }
  }

  EncodeOptions options;
  if (argEncoding == "HL") {
    options.verticalFrag = false;
    options.msb1st = false;
  } else if (argEncoding == "HM") {
    options.verticalFrag = false;
    options.msb1st = true;
  } else if (argEncoding == "VL") {
    options.verticalFrag = true;
    options.msb1st = false;
  } else if (argEncoding == "VM") {
    options.verticalFrag = true;
    options.msb1st = true;
  } else {
    fprintf(stderr, "Unknown encoding: %s\n", argEncoding.c_str());
    return 1;
  }
  options.noCpx = argNoCPX;
  options.noSfi = argNoSFI;
  options.forceZeroPadding = argForceZeroPadding;
  options.verbose = argVerbose;
  options.verboseForCode = argVerboseForCode;

  if (options.verbose) {
    std::cout << "MameFont Encoder" << std::endl;
    std::cout << "  Input   : " << argInput.c_str() << std::endl;
    std::cout << "  Output  : " << argOutput.c_str() << std::endl;
    std::cout << "  Encoding: " << argEncoding.c_str() << std::endl;
    std::cout << "  No CPX  : " << (argNoCPX ? "true" : "false") << std::endl;
    std::cout << "  No SFI  : " << (argNoSFI ? "true" : "false") << std::endl;
  }

  if (options.verbose) {
    std::cout << "Loading font from " << argInput.c_str() << "...\n";
  }

  BitmapFont bmpFont = std::make_shared<BitmapFontClass>(argInput);

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
    std::cout << "  Y spacing         : " << bmpFont->ySpacing << std::endl;
  }

  Encoder encoder(options);
  encoder.addFont(bmpFont);
  encoder.encode();
  encoder.generateBlob();

  mf::Font mameFont(encoder.blob.data());
  bool success = verifyGlyphs(bmpFont, mameFont, options.verbose, options.verboseForCode);
  return success ? 0 : 1;
}
