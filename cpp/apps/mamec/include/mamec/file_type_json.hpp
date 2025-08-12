#pragma once

#include <string>
#include <vector>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

std::string importJson(std::istream &is, std::vector<uint8_t> &blob);

void exportJson(std::ostream &os, const std::vector<uint8_t> &blob,
                std::string name);

}  // namespace mamefont::mamec
