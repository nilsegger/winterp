#include <gtest/gtest.h>

#include "instructions.hpp"
#include "runtime.hpp"
#include "sections.hpp"

class Test07 : public ::testing::Test {
protected:
  static WasmFile wasm;

  static void SetUpTestSuite() {
    EXPECT_EQ(wasm.read("test_binaries/07_test_bulk_memory.wasm"), 0);
  }

  static void TearDownTestSuite() {}
};

WasmFile Test07::wasm;

#define WASM_TEST(func_name, expected_value)                                   \
  TEST_F(Test07, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::I32);          \
                                                                               \
    EXPECT_EQ(static_cast<int32_t>(result.v.n32), expected_value);             \
  }


WASM_TEST(_test_fill_basic, 42);
// TODO: Again, here i think the test description is wrong, it stores the decimal byte 99, never 42, loading this byte should not convert it to 42?
WASM_TEST(_test_fill_range, 99);
WASM_TEST(_test_fill_single, 77);
WASM_TEST(_test_fill_zero, 0);
WASM_TEST(_test_copy_basic, 1819043144);
WASM_TEST(_test_copy_single, 65);
WASM_TEST(_test_copy_block, 170);
WASM_TEST(_test_copy_overlapping, 1);
WASM_TEST(_test_init_basic, 72);
WASM_TEST(_test_init_partial, 87);
WASM_TEST(_test_init_segment1, 3);
WASM_TEST(_test_drop_after_use, 72);
WASM_TEST(_test_combined_fill_copy, 55);
WASM_TEST(_test_combined_init_copy, 72);
WASM_TEST(_test_zero_length, 123);
