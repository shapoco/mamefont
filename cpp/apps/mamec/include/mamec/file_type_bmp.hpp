#pragma once

#include <string>
#include <vector>

#include "mamec/mamec_common.hpp"
#include "mamec/encoder.hpp"

namespace mamefont::mamec {

std::string importBitmapFont(const BitmapFont bmpFont, std::vector<uint8_t>& blob,
                      const EncodeOptions& options) ;

}  // namespace mamefont::mamec
