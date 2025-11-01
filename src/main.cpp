#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include "leb128.hpp"
#include "sections.hpp"

int main(int argn, char **argc) {

  if (argn < 2) {
    std::cout << "missing input file" << std::endl;
    return 1;
  }

  std::ifstream file(argc[1], std::ios::binary);

  if (!file) {
    std::cerr << "unable to open " << argc[1] << std::endl;
    return 1;
  }

  std::array<uint8_t, 8> header{}; // Read Magic and Version
  file.read((char *)header.data(), header.size());
  if (file.gcount() != 8) {
    std::cerr << argc[1] << "corruped file" << std::endl;
    return 1;
  }

  const unsigned char expected_magic[4] = {0x00, 0x61, 0x73, 0x6D};
  if (!std::equal(header.begin(), header.begin() + 4, expected_magic)) {
    std::cerr << "is not a valid WebAssembly binary file" << std::endl;
    return 1;
  }

  // Section information based on
  // https://webassembly.github.io/spec/core/binary/modules.html

  uint8_t section_id;
  uint32_t section_size;

  wasm wasm;

  do {
    file.read((char *)&section_id, sizeof(section_id));
    section_size = file_uleb128_u32t(file);

    std::cout << "; section " << section_name(section_id) << std::endl;
    std::cout << static_cast<int>(section_size) << "\t\t\t; section_size"
              << std::endl;

    // assumes code files are not larger that working memory...
    std::vector<uint8_t> section_data(section_size);
    file.read((char *)section_data.data(), section_size);

    // TODO: lookup table? only more ergonomic...
    if (section_id == TYPE_SECTION) {
      parse_type_section(wasm, section_data);
    } else if (section_id == FUNCTION_SECTION) {
      parse_functions(wasm, section_data);
    } else if (section_id == MEMORY_SECTION) {
      parse_memory(wasm, section_data);
    } else if (section_id == GLOBAL_SECTION) {
      parse_global(wasm, section_data);
    } else if (section_id == EXPORT_SECTION) {
      parse_exports(wasm, section_data);
    } else if (section_id == CODE_SECTION) {
      parse_code(wasm, section_data);
    }

  } while (file.peek() != EOF);

  // Summarise Functions

  for(auto& exp : wasm.exports) {

    if(exp.kind != ExportKind::func) {
      continue;
    }
    
    std::cout << exp.name << std::endl;
    Code& f = wasm.codes[exp.idx];

    for(auto& instr: f.expr) {
      std::cout << std::hex << instr.op << "\t\t";
      for(auto& imm: instr.imms) {
        std::cout << std::hex << imm.n32 << ",\t";
      }
      std::cout<<std::endl;
    }
  }

  return 0;
}
