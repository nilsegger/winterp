#ifndef ULEB_HPP
#define ULEB_HPP

#include <cstdint>
#include <fstream>

// Reads a ULEB128 integer directly from file stream
// Converts it to a uint32_t
uint32_t file_uleb128_u32t(std::ifstream &file);


// Starts reading the bytes at start and converts from uleb128 to uint32_t
uint32_t uleb128_u32t(const uint8_t* &start, const uint8_t* end);


#endif // ULEB_HPP

