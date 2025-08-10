#pragma once

#include <map>
#include <memory>

#include "mamec/bitmap_font.hpp"
#include "mamec/buffer_state.hpp"
#include "mamec/mame_glyph.hpp"
#include "mamec/mamec_common.hpp"
#include "mamec/operation.hpp"
#include "mamec/vec_ref.hpp"

namespace mamefont::mamec {

struct EncodeOptions {
  bool verbose = true;
  int verboseForCode = -1;
  bool verticalFrag = false;
  bool msb1st = false;
  bool noCpx = false;
  bool noSfi = false;
  bool forceZeroPadding = false;
};

struct TryContext {
  const int code;
  std::vector<Operation> &oprs;
  const BufferState &state;
  const VecRef &future;
  const VecRef &compareMask;
};

class Encoder {
 public:
  const EncodeOptions options;
  int fontHeight = 0;
  int ySpacing = 0;
  std::map<int, MameGlyph> glyphs;
  std::vector<frag_t> lut;
  std::vector<uint8_t> blob;

  Encoder(EncodeOptions opts) : options(opts){};

  void addGlyph(const BitmapFont &font, const BitmapGlyph &glyph);
  void addFont(const BitmapFont &font);
  void encode();
  void generateBlob();

 private:
  void generateInitialOperations(MameGlyph &glyph);
  void tryLDI(TryContext ctx);
  void tryXOR(TryContext ctx);
  void tryRPT(TryContext ctx);
  void trySFT(TryContext ctx);
  void trySFI(TryContext ctx);
  bool tryShiftCore(TryContext ctx, bool isSFI, bool right, bool postSet,
                    bool preShift, int size, int period);
  void tryCPY(TryContext ctx);
  void tryCPX(TryContext ctx);

  void generateLut();
  void replaceLDItoLUP();
  int findFragmentFromLUT(frag_t frag);
};
}  // namespace mamefont::mamec
