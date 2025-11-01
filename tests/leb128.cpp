#include <gtest/gtest.h>

#include "leb128.hpp"

TEST(LEB128, Decoding) {

  // Example from https://en.wikipedia.org/wiki/LEB128#Unsigned_LEB128
  std::vector<uint8_t> data = {0xE5, 0x8E, 0x26};
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();

  uint32_t result = uleb128_decode<uint32_t>(ptr, end);
  EXPECT_EQ(result, 624485);
}
