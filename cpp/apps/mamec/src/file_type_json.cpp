#include <sstream>

#include <nlohmann/json.hpp>

#include "mamec/file_type_json.hpp"
#include "mamec/mamec_common.hpp"

namespace mamefont::mamec {

std::string importJson( std::istream &is, std::vector<uint8_t> &blob) {
  nlohmann::json json = nlohmann::json::parse(is);

  if (json["format"] != "MameFont") {
    throw std::runtime_error("Invalid format");
  }

  std::string name = json["name"];
  blob = json["blob"].get<std::vector<uint8_t>>();

  return name;
}

void exportJson(std::ostream &os, const std::vector<uint8_t> &blob,
                std::string name) {
  mf::Font mameFont(blob.data());

  int glyphTableOffset = mf::FontHeader::SIZE;
  int fragTableOffset = mameFont.fragmentTableOffset();
  int byteCodeOffset = mameFont.byteCodeOffset();

  os << "{\n";
  os << "  \"format\": \"MameFont\",\n";
  os << "  \"name\": \"" << name << "\",\n";
  os << "  \"blob\": [\n";
  dumpCStyleArrayContent(os, blob, "    ", 0, mf::FontHeader::SIZE, false,
                         true);
  os << "    \n";
  dumpCStyleArrayContent(os, blob, "    ", glyphTableOffset,
                         fragTableOffset - glyphTableOffset, false, true);
  os << "    \n";
  dumpCStyleArrayContent(os, blob, "    ", fragTableOffset,
                         byteCodeOffset - fragTableOffset, false, true);
  os << "    \n";
  dumpCStyleArrayContent(os, blob, "    ", byteCodeOffset,
                         blob.size() - byteCodeOffset, false, false);
  os << "  ]\n";
  os << "}\n";
}

}  // namespace mamefont::mamec