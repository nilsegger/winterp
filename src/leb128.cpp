#include <cassert>
#include <cstring>

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

uint8_t read_byte(const uint8_t* &start, const uint8_t* end) {
  if(start == end) {
    assert(false && "Invalid byte read");
  }

  uint8_t value = *start;
  start++;
  return value;
}

// Reads a float from start based on the IEEE754 standard.
float read_float(const uint8_t* &start, const uint8_t* end) {

    if(end - start < 4) {
      assert(false && "Invalid floating number found.");
    }
  
    // TODO: this would fail if we are on a big-endian machine...
    float value;
    std::memcpy(&value, start, 4);
    start += 4;
    return value;  
}


// Reads a float from start based on the IEEE754 standard.
double read_double(const uint8_t* &start, const uint8_t* end) {

    if(end - start < 8) {
      assert(false && "Invalid double found.");
    }
  
    // TODO: this would fail if we are on a big-endian machine...
    double value;
    std::memcpy(&value, start, 8);
    start += 8;
    return value;  
}
