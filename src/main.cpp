#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include "sections.hpp"

uint32_t uleb128_u32t(std::ifstream &file) {

  // Read in bits of 8 until msb is 0
  // TODO: preallocate, known max size,
  // https://webassembly.github.io/spec/core/binary/values.html#integers
  std::vector<uint8_t> blocks;
  uint8_t block;

  do {
    file.read((char *)&block, 1);
    blocks.push_back(block);
  } while ((block & 0b10000000) > 0);

  uint32_t result = 0;
  for (int i = 0; i < blocks.size(); i++) {
    // set msb to always be 0 such that it doesnt contribute, then shift
    uint8_t block = blocks[i] & 0x7F;
    int shift = i * 7;
    result |= (block << shift);
  }

  return result;
}


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

  do {
    file.read((char *)&section_id, sizeof(section_id));
    section_size = uleb128_u32t(file);

    std::cout << "; section " << section_name(section_id) << std::endl;
    std::cout << static_cast<int>(section_size) << "\t\t\t section_size"
              << std::endl;

    file.seekg(section_size, std::ios::cur);
  } while (file.peek() != EOF);

  return 0;
}
