#pragma once

#include <map>
#include <sstream>
#include <vector>

#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

void dumpMetrics(const std::vector<uint8_t> &blob, std::ostream &os,
                 const std::string &indent);

}  // namespace mamec
