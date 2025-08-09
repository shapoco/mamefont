#pragma once

#include <mamefont/mamefont.hpp>
#include "mamec/bitmap_font.hpp"

namespace mamefont::mamec {

bool verifyGlyphs(const BitmapFont &bmpFont, const mf::Font &mameFont, bool verbose);

}
