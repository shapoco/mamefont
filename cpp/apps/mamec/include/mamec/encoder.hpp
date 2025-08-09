#pragma once

#include <map>
#include <memory>

#include "mamec/bitmap_font.hpp"
#include "mamec/generation_state.hpp"
#include "mamec/mame_glyph.hpp"
#include "mamec/mamec_common.hpp"
#include "mamec/operation.hpp"

namespace mamefont::mamec {

struct EncodeOptions {
  bool verbose = true;
  int verboseForCode = -1;
  bool verticalFrag = false;
  bool msb1st = false;
  bool noCpx = false;
  bool noSfi = false;
};

class Encoder {
 public:
  const EncodeOptions options;
  int fontHeight = 0;
  int ySpacing = 0;
  std::map<int, MameGlyph> glyphs;
  std::vector<fragment_t> lut;
  std::vector<uint8_t> blob;

  Encoder(EncodeOptions opts) : options(opts){};

  void addGlyph(const BitmapFont &font, const BitmapGlyph &glyph);
  void addFont(const BitmapFont &font);
  void encode();
  void generateBlob();

 private:
  void generateInitialOperations(MameGlyph &glyph);
  void tryLDI(std::vector<Operation> &oprs, GenerationState &state,
              const VecRef &future, const VecRef &mask);
  void tryRPT(std::vector<Operation> &oprs, GenerationState &state,
              const VecRef &future, const VecRef &mask);
  void generateLut();
  void replaceLDItoLUP();
  int findFragmentFromLUT(fragment_t frag);
};
}  // namespace mamefont::mamec
