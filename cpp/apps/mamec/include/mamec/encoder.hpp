#pragma once

#include <map>
#include <memory>

#include "mamec/bitmap_font.hpp"
#include "mamec/buffer_state.hpp"
#include "mamec/glyph_object.hpp"
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
  int glyphHeight = 0;
  int ySpacing = 0;
  std::map<int, GlyphObject> glyphs;
  std::vector<frag_t> fragTable;
  std::map<frag_t, int> ldiFrags;
  std::vector<uint8_t> blob;

  Encoder(EncodeOptions opts) : options(opts){};

  void addGlyph(const BitmapFont &font, const BitmapGlyph &glyph);
  void addFont(const BitmapFont &font);
  void encode();
  void generateBlob();

 private:
  void generateInitialOperations(GlyphObject &glyph, bool verbose = false,
                                 std::string indent = "");
  void tryLUP(TryContext ctx);
  void tryXOR(TryContext ctx);
  void tryRPT(TryContext ctx);
  void trySFT(TryContext ctx);
  void trySFI(TryContext ctx);
  bool tryShiftCore(TryContext ctx, bool isSFI, bool right, bool postSet,
                    bool preShift, int size, int period);
  void tryCPY(TryContext ctx);
  void tryCPX(TryContext ctx);

  void generateInitialFragTable();
  void generateFullFragTable();
  void generateFragTableFromCountMap(std::map<frag_t, int> &countMap, int tableSize);
  void optimizeFragmentTable();
  void fixLUPIndex();
  void replaceLDItoLUP(bool verbose = false, std::string indent = "");
  int reverseLookup(frag_t frag);
};
}  // namespace mamefont::mamec
