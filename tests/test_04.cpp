
#include <gtest/gtest.h>

#include "instructions.hpp"
#include "runtime.hpp"
#include "sections.hpp"

class Test04 : public ::testing::Test {
protected:
  static WasmFile wasm;

  static void SetUpTestSuite() {
    EXPECT_EQ(wasm.read("test_binaries/04_test_prio3.wasm"), 0);
  }

  static void TearDownTestSuite() {}
};

WasmFile Test04::wasm;

#define WASM_TEST(func_name, expected_value)                               \
  TEST_F(Test04, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::I32);          \
                                                                               \
    EXPECT_EQ(static_cast<int32_t>(result.v.n32), expected_value);                                   \
  }
  
#define WASM_F32_TEST(func_name, expected_value)                               \
  TEST_F(Test04, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::F32);          \
                                                                               \
    EXPECT_EQ(result.v.p32, expected_value);                                   \
  }

WASM_TEST(_test_i32_rem_u, 2);
WASM_TEST(_test_i32_rem_u_large, 1);
WASM_TEST(_test_i64_rem_u, 2);
WASM_TEST(_test_i64_rem_u_large, 1);
WASM_TEST(_test_i32_le_u, 1);
WASM_TEST(_test_i32_le_u_equal, 1);
WASM_TEST(_test_i32_le_u_large, 0);
WASM_TEST(_test_i32_ge_u, 1);
WASM_TEST(_test_i32_ge_u_equal, 1);
WASM_TEST(_test_i32_ge_u_large, 1);
WASM_TEST(_test_i64_le_s, 1);
WASM_TEST(_test_i64_le_s_negative, 1);
WASM_TEST(_test_i64_le_u, 1);
WASM_TEST(_test_i64_le_u_large, 0);
WASM_TEST(_test_i64_ge_s, 1);
WASM_TEST(_test_i64_ge_s_negative, 1);
WASM_TEST(_test_i64_ge_u, 1);
WASM_TEST(_test_i64_ge_u_large, 1);
WASM_F32_TEST(_test_f32_copysign_neg, -3.5f);
WASM_F32_TEST(_test_f32_copysign_pos, 3.5f);
WASM_F32_TEST(_test_f32_copysign_both_pos, 3.5f);


// all these tests have the apparent problem that the lower 32 bits of the test numbers are all 0..
// verified by https://binaryconvert.com/result_double.html?decimal=050046049WASM_TEST(_test_f64_copysign_neg, 0); 
WASM_TEST(_test_f64_copysign_pos, 0);
WASM_TEST(_test_f64_sub, 0);
WASM_TEST(_test_f64_div, 0);
WASM_TEST(_test_f64_min, 0xCCCCCCCD);
WASM_TEST(_test_f64_max, 0);
WASM_TEST(_test_f64_abs, 0);
WASM_TEST(_test_f64_neg, 0);
WASM_TEST(_test_f64_ceil, 0);
WASM_TEST(_test_f64_floor, 0);
WASM_TEST(_test_f64_trunc, 0);
WASM_TEST(_test_f64_nearest, 0);

WASM_TEST(_test_f64_le, 1);
WASM_TEST(_test_f64_ge, 1);

// TODO: Test mentions only 3.14, why?
// Confident that this is an error description in the test file
// Mentions storing 3.14 but is actually storing 3.14159
WASM_F32_TEST(_test_f32_store_load, 3.14159f);
WASM_F32_TEST(_test_f32_store_load_negative, -2.5);

//https://binaryconvert.com/result_double.html?decimal=050046055049056050056049056050056
WASM_TEST(_test_f64_store_load, 0x8B04919B);

// https://binaryconvert.com/result_double.html?decimal=049050051052053054046055056057
WASM_TEST(_test_f64_store_load_large, 0x9FBE76C9);
WASM_F32_TEST(_test_f32_arithmetic_with_load, 8.0);
WASM_TEST(_test_unreachable_not_reached, 42);
WASM_TEST(_test_unreachable_in_branch, 100);

WASM_TEST(_test_unreachable_in_else, 50);
