#pragma once

#include <string>
#include <vector>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

void exportHpp(std::ostream &os, const std::vector<uint8_t> &blob,
               std::string name);

}  // namespace mamefont::mamec
