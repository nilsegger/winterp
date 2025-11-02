#include <gtest/gtest.h>

#include "instructions.hpp"
#include "runtime.hpp"
#include "sections.hpp"

class Test01 : public ::testing::Test {
protected:
  static WasmFile wasm;

  static void SetUpTestSuite() {
    EXPECT_EQ(wasm.read("01_test.wasm"), 0);
  }

  static void TearDownTestSuite() {}
};

WasmFile Test01::wasm;

#define WASM_TEST(func_name, expected_value)                               \
  TEST_F(Test01, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::I32);          \
                                                                               \
    EXPECT_EQ(static_cast<int32_t>(result.v.n32), expected_value);                                   \
  }
  
#define WASM_F32_TEST(func_name, expected_value)                               \
  TEST_F(Test01, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::F32);          \
                                                                               \
    EXPECT_EQ(result.v.p32, expected_value);                                   \
  }

WASM_TEST(_test_store, 42);
WASM_TEST(_test_addition, 15);
WASM_TEST(_test_subtraction, 12);
WASM_TEST(_test_multiplication, 42);
WASM_TEST(_test_division_signed, 5);
WASM_TEST(_test_division_unsigned, 6);
WASM_TEST(_test_remainder, 2);
WASM_TEST(_test_and, 10);
WASM_TEST(_test_or, 14);
WASM_TEST(_test_xor, 6);
WASM_TEST(_test_shift_left, 20);
WASM_TEST(_test_shift_right_signed, 0xFFFFFFFC);
WASM_TEST(_test_shift_right_unsigned, 4);
WASM_TEST(_test_store_load, 99);
WASM_TEST(_test_store_load_byte_unsigned, 255);
WASM_TEST(_test_store_load_byte_signed, 0xFFFFFFFF);
WASM_TEST(_test_locals_arithmetic, 35);
WASM_TEST(_test_locals_tee, 15);
WASM_TEST(_test_global_increment, 1);
WASM_TEST(_test_global_constant, 100);
WASM_TEST(_test_global_multiple, 10);
WASM_TEST(_test_combined, 142);
WASM_TEST(_test_eq, 1);
WASM_TEST(_test_ne, 1);
WASM_TEST(_test_lt_s, 1);
WASM_TEST(_test_lt_u, 1);
WASM_TEST(_test_gt_s, 1);
WASM_TEST(_test_gt_u, 1);
WASM_TEST(_test_le_s, 1);
WASM_TEST(_test_ge_s, 1);
WASM_TEST(_test_eqz_zero, 1);
WASM_TEST(_test_eqz_nonzero, 0);
WASM_TEST(_test_clz, 28);
WASM_TEST(_test_ctz, 3);
WASM_TEST(_test_popcnt, 3);
WASM_TEST(_test_popcnt_all, 32);
WASM_TEST(_test_rotl, 16);
WASM_TEST(_test_rotr, 1);
WASM_TEST(_test_rotl_wrap, 1);
WASM_TEST(_test_load16_u, 65535);
WASM_TEST(_test_load16_s, 0xFFFFFFFF);
WASM_TEST(_test_load16_32768, 32768);
WASM_TEST(_test_select_true, 10);
WASM_TEST(_test_select_false, 20);
WASM_TEST(_test_if_true, 100);
WASM_TEST(_test_if_false, 200);
WASM_TEST(_test_if_no_else, 50);
WASM_TEST(_test_nested_if, 1);
WASM_TEST(_test_block_break, 10);
WASM_TEST(_test_block_no_break, 20);
WASM_TEST(_test_loop_sum, 15);
WASM_TEST(_test_loop_early_break, 15);
WASM_TEST(_test_br_table_case0, 100);
WASM_TEST(_test_br_table_case2, 102);
