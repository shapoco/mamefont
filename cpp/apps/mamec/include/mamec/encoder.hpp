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
  bool farPixelFirst = false;
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
  mf::PixelFormat pixelFormat = mf::PixelFormat::BW_1BIT;
  std::map<int, GlyphObject> glyphs;
  std::vector<frag_t> fragTable;
  std::map<frag_t, int> ldiFrags;
  std::vector<uint8_t> blob;

  Encoder(EncodeOptions opts) : options(opts){};

  void addGlyph(const BitmapFont &font, const BitmapGlyph &glyph);
  void addFont(const BitmapFont &font);
  void encode();
  void generateBlob();

  static uint16_t encodeShiftState2bpp(frag_t frag, bool right, bool postSet) {
    constexpr uint8_t LUT[3][4] = {
        {0b000, 0b001, 0b011, 0b111},
        {0b000, 0b010, 0b101, 0b111},
        {0b000, 0b100, 0b110, 0b111},
    };

    uint8_t tmp[6] = {0};
    if (right) {
      tmp[0] = frag & 3;
      tmp[5] = postSet ? 3 : 0;
    } else {
      tmp[0] = postSet ? 3 : 0;
      tmp[5] = (frag >> 6) & 3;
    }

    for (int i = 0; i < 4; i++) {
      tmp[i + 1] = (frag >> (i * 2)) & 3;
    }

    uint16_t ret = 0;
    for (int i = 0; i < 4; i++) {
      int cmp;
      if (tmp[i + 2] < tmp[i]) {
        cmp = 0;
      } else if (tmp[i + 2] == tmp[i]) {
        cmp = 1;
      } else {
        cmp = 2;
      }
      uint16_t val = LUT[cmp][(frag >> (i * 2)) & 3];
      ret |= (val << (i * 3));
    }
    return ret;
  }

  static frag_t decodeShiftState2bpp(uint16_t state) {
    constexpr uint8_t LUT[8] = {
        0, 1, 1, 2, 1, 2, 2, 3,
    };

    frag_t ret = 0;
    for (int i = 0; i < 4; i++) {
      uint8_t val = (state >> (i * 3)) & 0b111;
      ret |= (LUT[val] << (i * 2));
    }
    return ret;
  }

 private:
  void detectFragmentDuplications(std::string indent);
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
  void generateFragTableFromCountMap(std::map<frag_t, int> &countMap,
                                     int tableSize);
  void optimizeFragmentTable();
  void fixLUPIndex();
  void replaceLDItoLUP(bool verbose = false, std::string indent = "");
  int reverseLookup(frag_t frag);
  void checkFragmentDuplicationSolvable();

};
}  // namespace mamefont::mamec
