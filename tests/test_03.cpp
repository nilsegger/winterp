#include <gtest/gtest.h>

#include "instructions.hpp"
#include "runtime.hpp"
#include "sections.hpp"

class Test03 : public ::testing::Test {
protected:
  static WasmFile wasm;

  static void SetUpTestSuite() {
    EXPECT_EQ(wasm.read("test_binaries/03_test_prio2.wasm"), 0);
  }

  static void TearDownTestSuite() {}
};

WasmFile Test03::wasm;

// TODO: test macros should be unified in a common file

#define WASM_TEST(func_name, offset, expected_value)                               \
  TEST_F(Test03, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, offset, ImmediateRepr::I32);          \
                                                                               \
    EXPECT_EQ(static_cast<int32_t>(result.v.n32), expected_value);                                   \
  }
  
#define WASM_F32_TEST(func_name, offset, expected_value)                               \
  TEST_F(Test03, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, offset, ImmediateRepr::F32);          \
                                                                               \
    EXPECT_EQ(result.v.p32, expected_value);                                   \
  }

WASM_TEST(_test_data_read_char_h, 200, 'H');
WASM_TEST(_test_data_read_char_e, 200, 'e');
WASM_TEST(_test_data_read_i32_42, 200, 42);
WASM_TEST(_test_data_read_i32_255, 200, 255);
WASM_TEST(_test_data_read_char_t, 200, 'T');
WASM_TEST(_test_data_read_exclaim, 200, '!');
WASM_TEST(_test_call_indirect_add, 200, 15);
WASM_TEST(_test_call_indirect_sub, 200, 5);
WASM_TEST(_test_call_indirect_mul, 200, 50);
WASM_TEST(_test_call_indirect_div, 200, 5);
WASM_TEST(_test_call_indirect_dynamic, 200, 50);
WASM_TEST(_test_call_indirect_loop, 200, 8);
WASM_TEST(_test_i64_add, 200, 15);
WASM_TEST(_test_i64_sub, 200, 12);
WASM_TEST(_test_i64_mul, 200, 42);
WASM_TEST(_test_i64_div_s, 200, 5);
WASM_TEST(_test_i64_div_u, 200, 6);
WASM_TEST(_test_i64_rem_s, 200, 2);
WASM_TEST(_test_i64_and, 200, 10);
WASM_TEST(_test_i64_or, 200, 15);
WASM_TEST(_test_i64_xor, 200, 15);
WASM_TEST(_test_i64_shl, 200, 20);
// lower bits taken from https://devtools.calckit.io/programmer-calculator
 WASM_TEST(_test_i64_shr_s, 200, 0xfffffffc);
WASM_TEST(_test_i64_shr_u, 200, 4);
WASM_TEST(_test_i64_rotl, 200, 16);
WASM_TEST(_test_i64_rotr, 200, 1);
WASM_TEST(_test_i64_clz, 200, 60);
WASM_TEST(_test_i64_ctz, 200, 3);
WASM_TEST(_test_i64_popcnt, 200, 3);
WASM_TEST(_test_i64_eq, 200, 1);
WASM_TEST(_test_i64_ne, 200, 1);
WASM_TEST(_test_i64_lt_s, 200, 1);
WASM_TEST(_test_i64_gt_s, 200, 1);
WASM_TEST(_test_i64_eqz, 200, 1);

WASM_TEST(_test_i64_extend_i32_s, 200, -1); 
WASM_TEST(_test_i64_extend_i32_u, 200, 255); 
WASM_TEST(_test_i64_wrap, 200, 255);

WASM_TEST(_test_i64_trunc_f32_s, 200, 42);
WASM_TEST(_test_i64_trunc_f64_s, 200, 100);
WASM_F32_TEST(_test_i64_convert_to_f32, 200, 42.0);
WASM_TEST(_test_i64_convert_to_f64, 200, 0); // lower bits of 100.0
WASM_TEST(_test_i64_store_load, 200, 255);
WASM_TEST(_test_i64_load32_u, 200, 0xF0);
WASM_TEST(_test_i64_load32_s, 200, 0x80000000);
WASM_TEST(_test_i64_call_function, 200, 42);
WASM_TEST(_test_i64_large_mul, 200, static_cast<int32_t>(1000000L * 1000L));
WASM_TEST(_test_i64_bit_pattern, 200, 0xFFFFFFFF);

/* TODO: IMPLEMENT TRAPPING */
 // WASM_TEST(_test_trap_safe_div, 0);
// WASM_TEST(_test_trap_divisor_zero, 0);
// WASM_TEST(_test_trap_check_div_zero, 0);
// WASM_TEST(_test_trap_check_mem_valid, 0);
// WASM_TEST(_test_trap_check_mem_invalid, 0);
// WASM_TEST(_test_trap_check_overflow, 0);
// WASM_TEST(_test_trap_check_rem_zero, 0);
// WASM_TEST(_test_trap_check_i64_div_zero, 0);

WASM_TEST(_test_combined_data_i64, 200, 84);
WASM_TEST(_test_combined_indirect_i64, 200, 30);
WASM_TEST(_test_combined_all_features, 200, 114);
