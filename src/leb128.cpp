#include "leb128.hpp"

uint32_t file_uleb128_u32t(std::ifstream &file) {

  // Read in bits of 8 until msb is 0
  // TODO: preallocate, known max size,
  // https://webassembly.github.io/spec/core/binary/values.html#integers
  std::vector<uint8_t> blocks;
  uint8_t block;

  do {
    file.read((char *)&block, 1);
    blocks.push_back(block);
  } while ((block & 0x80) > 0);

  return uleb128_from_blocks<uint32_t>(blocks);
}

