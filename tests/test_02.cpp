#include <gtest/gtest.h>

#include "instructions.hpp"
#include "runtime.hpp"
#include "sections.hpp"

class Test02 : public ::testing::Test {
protected:
  static WasmFile wasm;

  static void SetUpTestSuite() {
    EXPECT_EQ(wasm.read("02_test_prio1.wasm"), 0);
  }

  static void TearDownTestSuite() {}
};

WasmFile Test02::wasm;

#define WASM_I32_TEST(func_name, expected_value)                               \
  TEST_F(Test02, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::I32);          \
                                                                               \
    EXPECT_EQ(result.v.n32, expected_value);                                   \
    EXPECT_EQ(static_cast<int32_t>(result.v.n32), expected_value);             \
  }

#define WASM_F32_TEST(func_name, expected_value)                               \
  TEST_F(Test02, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::F32);          \
                                                                               \
    EXPECT_EQ(result.v.p32, expected_value);                                   \
  }

#define WASM_F64_TEST(func_name, expected_value)                               \
  TEST_F(Test02, func_name) {                                                  \
    std::string func = #func_name;                                             \
                                                                               \
    Runtime runtime(wasm);                                                     \
    runtime.run(func);                                                         \
    Immediate result = runtime.read_memory(2, 0, ImmediateRepr::F64);          \
                                                                               \
    EXPECT_EQ(result.v.p64, expected_value);                                   \
  }

WASM_I32_TEST(_test_call_add, 15);
WASM_I32_TEST(_test_call_composition, 35);
WASM_I32_TEST(_test_call_square, 49);
WASM_I32_TEST(_test_call_multiple, 25);
WASM_I32_TEST(_test_return_early_true, 100);
WASM_I32_TEST(_test_return_early_false, 200);
WASM_I32_TEST(_test_abs_negative, 42);
WASM_I32_TEST(_test_abs_positive, 42);
WASM_I32_TEST(_test_factorial, 120);
WASM_F32_TEST(_test_f32_add, 6.0f);
WASM_F32_TEST(_test_f32_sub, 7.0f);
WASM_F32_TEST(_test_f32_mul, 10.0f);
WASM_F32_TEST(_test_f32_div, 2.5f);
WASM_F32_TEST(_test_f32_min, 2.1f);
WASM_F32_TEST(_test_f32_max, 3.5f);
WASM_F32_TEST(_test_f32_abs, 3.5);
WASM_F32_TEST(_test_f32_neg, -3.5);
WASM_F32_TEST(_test_f32_sqrt, 4.0);
WASM_F32_TEST(_test_f32_ceil, 4.0);
WASM_F32_TEST(_test_f32_floor, 3.0);
WASM_F32_TEST(_test_f32_trunc, 3.0);
WASM_F32_TEST(_test_f32_nearest, 4.0);
WASM_I32_TEST(_test_f32_eq, 1);
WASM_I32_TEST(_test_f32_ne, 1);
WASM_I32_TEST(_test_f32_lt, 1);
WASM_I32_TEST(_test_f32_gt, 1);
WASM_I32_TEST(_test_f32_le, 1);
WASM_I32_TEST(_test_f32_ge, 1);
WASM_F32_TEST(_test_f32_call, 4.0);


// Problem with these tests are that for al of them the lower 32 bits are 0...
WASM_I32_TEST(_test_f64_add, 0.0);
WASM_I32_TEST(_test_f64_mul, 0.0);
WASM_I32_TEST(_test_f64_sqrt, 0.0);
WASM_I32_TEST(_test_f64_gt, 1);

WASM_F32_TEST(_test_convert_i32_to_f32_s, 42.0);
WASM_F32_TEST(_test_convert_i32_to_f32_u, 42.0);
WASM_I32_TEST(_test_convert_f32_to_i32_s, 42);
WASM_I32_TEST(_test_convert_f32_to_i32_u, 42);
// again, lower bits will be 0...
WASM_I32_TEST(_test_convert_i32_to_f64_s, 0);
WASM_I32_TEST(_test_convert_f64_to_i32_s, 100);
WASM_I32_TEST(_test_promote_f32_to_f64, 0);
WASM_F32_TEST(_test_demote_f64_to_f32, 3.5);
WASM_I32_TEST(_test_reinterpret_f32_to_i32, 0x3F800000);
WASM_I32_TEST(_test_reinterpret_i32_to_f32, 0x40400000);

WASM_I32_TEST(_test_drop_simple, 42);
WASM_I32_TEST(_test_drop_multiple, 100);
WASM_I32_TEST(_test_nop, 42);
WASM_I32_TEST(_test_drop_in_computation, 50);
// WASM_I32_TEST(_test_memory_size, 0);
// WASM_I32_TEST(_test_memory_grow, 0);
// WASM_I32_TEST(_test_memory_size_after_grow, 0);
// WASM_I32_TEST(_test_memory_grow_multiple, 0);
// WASM_I32_TEST(_test_memory_write_grown, 0);
// WASM_I32_TEST(_test_combined_functions, 0);
// WASM_I32_TEST(_test_combined_float_convert, 0);
