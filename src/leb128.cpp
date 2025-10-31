#include <vector>

#include "leb128.hpp"


uint32_t uin32t_from_blocks(const std::vector<uint8_t>& blocks) {
  uint32_t result = 0;
  for (int i = 0; i < blocks.size(); i++) {
    // set msb to always be 0 such that it doesnt contribute, then shift
    uint8_t block = blocks[i] & 0x7F;
    int shift = i * 7;
    result |= (block << shift);
  }

  return result;
}

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

  return uin32t_from_blocks(blocks);
}

uint32_t uleb128_u32t(const uint8_t* &start, const uint8_t* end) {

  // Read in bits of 8 until msb is 0
  // TODO: preallocate, known max size,
  // https://webassembly.github.io/spec/core/binary/values.html#integers
  std::vector<uint8_t> blocks;
  uint8_t block;

  while(start != end) {

    blocks.push_back(*start);

    bool is_end = ((*start) & 0x80) == 0;
    start++; // Increase before exiting, such that next call to this function starts at the next block

    if(is_end) {
      break;
    }
  }

  return uin32t_from_blocks(blocks);
}
