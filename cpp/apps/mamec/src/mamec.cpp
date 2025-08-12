#include <filesystem>
#include <fstream>
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
static constexpr char OPT_VERIFY_ONLY = 'v';
static constexpr char OPT_NO_CPX = 0x82;
static constexpr char OPT_NO_SFI = 0x83;
static constexpr char OPT_FORCE_ZERO_PADDING = 0x84;
static constexpr char OPT_VERBOSE = 0x85;

static struct option long_opts[] = {
    {"input", required_argument, 0, OPT_INPUT},
    {"output", required_argument, 0, OPT_OUTPUT},
    {"encoding", required_argument, 0, OPT_ENCODING},
    {"verify_only", no_argument, 0, OPT_VERIFY_ONLY},
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
  bool argVerifyOnly = false;
  std::string argVerboseForCodeStr;
  int argVerboseForCode = -1;

  char short_opts[256];
  snprintf(short_opts, sizeof(short_opts), "%c:%c:%c:%c", OPT_INPUT, OPT_OUTPUT,
           OPT_ENCODING, OPT_VERIFY_ONLY);

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
      case OPT_VERIFY_ONLY:
        argVerifyOnly = true;
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
      try {
        argVerboseForCode = std::stoi(argVerboseForCodeStr, nullptr, 16);
      } catch (const std::exception &e) {
        std::cerr << "*WARNING: Invalid verbose code: " << argVerboseForCodeStr
                  << std::endl;
      }
    } else {
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

  FileType outputFileType = FileType::NONE;
  if (argVerifyOnly) {
    if (!argOutput.empty()) {
      std::cerr << "*ERROR: Output file cannot be specified in verify mode."
                << std::endl;
    }
  } else {
    if (argOutput.empty()) {
      std::cerr << "*ERROR: Output file must be specified." << std::endl;
      return 1;
    } else if (argOutput.ends_with(".json")) {
      outputFileType = FileType::MAME_JSON;
    } else if (argOutput.ends_with(".hpp")) {
      outputFileType = FileType::MAME_HPP;
    } else {
      std::cerr << "*ERROR: Unknown output file extension." << std::endl;
      return 1;
    }
  }

  if (argInput == argOutput) {
    std::cerr << "*ERROR: Input and output files cannot be the same."
              << std::endl;
    return 1;
  }

  std::string fontName;
  std::vector<uint8_t> blob;
  std::shared_ptr<mf::Font> mameFont = nullptr;
  BitmapFont bmpFont = nullptr;

  bool success = true;
  try {
    if (options.verbose) {
      std::cout << "Loading font from " << argInput.c_str() << "...\n";
    }

    if (argInput.ends_with(".bmp") || argInput.ends_with(".png") ||
        argInput.ends_with(".jpg") || argInput.ends_with(".jpeg")) {
      bmpFont = std::make_shared<BitmapFontClass>(argInput);
      fontName = importBitmapFont(bmpFont, blob, options);
    } else if (argInput.ends_with(".json")) {
      std::ifstream ifs(argInput);
      if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open input file: " + argInput);
      }
      fontName = importJson(ifs, blob);
      ifs.close();
    } else {
      throw std::runtime_error("Unknown input file type: " + argInput);
    }

    if (bmpFont) {
      mf::Font mameFont(blob.data());
      if (!verifyGlyphs(bmpFont, mameFont, options.verbose,
                        options.verboseForCode)) {
        throw std::runtime_error("Glyph verification failed");
      }
    }

    if (!argVerifyOnly && success) {
      switch (outputFileType) {
        case FileType::MAME_JSON: {
          std::ofstream ofs(argOutput);
          exportJson(ofs, blob, fontName);
          ofs.close();
        } break;

        case FileType::MAME_HPP: {
          std::ofstream ofs(argOutput);
          exportHpp(ofs, blob, fontName);
          ofs.close();
        } break;

        default:
          throw std::runtime_error("Unknown output file type");
      }
    }
  } catch (const std::exception &e) {
    success = false;
    std::cerr << "*ERROR: " << e.what() << std::endl;
  }

  return success ? 0 : 1;
}
