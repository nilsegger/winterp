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

    static void TearDownTestSuite() {
    }
};

WasmFile Test02::wasm;

#define WASM_I32_TEST(func_name, expected_value)                       \
TEST_F(Test02, func_name) {                                            \
  std::string func = #func_name;                                       \
                                                                       \
  Runtime runtime(wasm);                                               \
  runtime.run(func);                                           \
  Immediate result = runtime.read_memory(2, 0, ImmediateRepr::I32);   \
                                                                       \
  EXPECT_EQ(result.v.n32, expected_value);                                           \
  EXPECT_EQ(static_cast<int32_t>(result.v.n32), expected_value);                                           \
}


WASM_I32_TEST(_test_call_add, 15);
WASM_I32_TEST(_test_call_composition, 35);
WASM_I32_TEST(_test_call_square, 49);
WASM_I32_TEST(_test_call_multiple, 25);
WASM_I32_TEST(_test_return_early_true, 100);
WASM_I32_TEST(_test_return_early_false, 200);
WASM_I32_TEST(_test_abs_negative, 42);
WASM_I32_TEST(_test_abs_positive, 42);
