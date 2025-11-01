#ifndef ULEB_HPP
#define ULEB_HPP

#include <cstdint>
#include <fstream>
#include <vector>


template<typename T>
T uleb128_from_blocks(const std::vector<uint8_t>& blocks) {
  T result = 0;
  for (int i = 0; i < blocks.size(); i++) {
    // set msb to always be 0 such that it doesnt contribute, then shift
    uint8_t block = blocks[i] & 0x7F;
    int shift = i * 7;
    result |= (block << shift);
  }

  return result;
}

template<typename T>
T uleb128_decode(const uint8_t* &start, const uint8_t* end) {

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

  return uleb128_from_blocks<T>(blocks);
}


// Reads a ULEB128 integer directly from file stream
// Converts it to a uint32_t
uint32_t file_uleb128_u32t(std::ifstream &file);


// Starts reading the bytes at start and converts from uleb128 to uint32_t
uint32_t uleb128_u32t(const uint8_t* &start, const uint8_t* end);

// Starts reading the bytes at start and converts from uleb128 to uint64_t
uint64_t uleb128_u64t(const uint8_t* &start, const uint8_t* end);

// Reads a single byte without any conversion
uint8_t read_byte(const uint8_t* &start, const uint8_t* end);

// Reads a float from start based on the IEEE754 standard.
// and moves the start pointer to its end
float read_float(const uint8_t* &start, const uint8_t* end);

// Reads a double from start based on the IEEE754 standard.
double read_double(const uint8_t* &start, const uint8_t* end);

#endif // ULEB_HPP

