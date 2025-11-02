
#include <gtest/gtest.h>

#include "instructions.hpp"
#include "runtime.hpp"
#include "sections.hpp"

class Test05 : public ::testing::Test {
protected:
  static WasmFile wasm;

  static void SetUpTestSuite() {
    EXPECT_EQ(wasm.read("05_test_complex.wasm"), 0);
  }

  static void TearDownTestSuite() {}
};

WasmFile Test05::wasm;

#define WASM_TEST(func_name, expected_value)                               \
  TEST_F(Test05, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::I32);          \
                                                                               \
    EXPECT_EQ(static_cast<int32_t>(result.v.n32), expected_value);                                   \
  }
  
#define WASM_F32_TEST(func_name, expected_value)                               \
  TEST_F(Test05, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::F32);          \
                                                                               \
    EXPECT_EQ(result.v.p32, expected_value);                                   \
  }

WASM_TEST(nested_blocks, 42);
WASM_TEST(block_results, 50);
WASM_TEST(conditional_nested_0, 100);
WASM_TEST(conditional_nested_1, 200);
WASM_TEST(conditional_nested_2, 300);
WASM_TEST(call_in_block, 42);
WASM_TEST(loop_with_blocks, 5);
WASM_TEST(multi_call, 30);
WASM_TEST(br_table_nested_0, 400);
WASM_TEST(br_table_nested_1, 300);
WASM_TEST(br_table_nested_2, 200);
WASM_TEST(br_table_nested_3, 100);
