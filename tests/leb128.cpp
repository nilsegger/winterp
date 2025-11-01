#include <gtest/gtest.h>

#include "leb128.hpp"

TEST(LEB128, DecodingUnsigned) {

  // Example from https://en.wikipedia.org/wiki/LEB128#Unsigned_LEB128
  std::vector<uint8_t> data = {0xE5, 0x8E, 0x26};
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();

  uint32_t result = uleb128_decode<uint32_t>(ptr, end);
  EXPECT_EQ(result, 624485);
}

TEST(LEB128, DecodingSigned) {

  // Example from https://en.wikipedia.org/wiki/LEB128#Unsigned_LEB128
  // with garbage data at the end
  std::vector<uint8_t> data = {0xC0, 0xBB, 0x78, 0x10, 0x00};
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();

  int32_t result = leb128_decode<int32_t>(ptr, end);
  EXPECT_EQ(result, -123456);
}

TEST(LEB128, DecodingNegativeSigned) {
  // Example from 02_test_prio1.wasm
  std::vector<uint8_t> data = {0x56};
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();

  int32_t result = leb128_decode<int32_t>(ptr, end);
  EXPECT_EQ(result, -42);
}
