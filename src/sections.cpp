#include "sections.hpp"

const char *section_name(uint8_t section) {
  switch (section) {
  case 0:
    return "custom";
  case 1:
    return "type";
  case 2:
    return "import";
  case 3:
    return "function";
  case 4:
    return "table";
  case 5:
    return "memory";
  case 6:
    return "global";
  case 7:
    return "export";
  case 8:
    return "start";
  case 9:
    return "element";
  case 10:
    return "code";
  case 11:
    return "data";
  case 12:
    return "data count";
  case 13:
    return "tag";
  default:
    return "unknown";
  }
}
