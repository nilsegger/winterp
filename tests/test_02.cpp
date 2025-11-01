#include <gtest/gtest.h>

#include "instructions.hpp"
#include "runtime.hpp"
#include "sections.hpp"

TEST(test02, _test_call_add) {
  WasmFile wasm;
  EXPECT_EQ(wasm.read("02_test_prio1.wasm"), 0);

  std::string func = "_test_call_add";
  ImmediateRepr result_repr;
  Immediate result;

  
  Runtime runtime(wasm);
  runtime.run(func, result_repr, result);

  EXPECT_EQ(runtime.read_memory(2, 0, ImmediateRepr::I32).n32, 15);
}
 
TEST(test02, _test_call_composition) {
  WasmFile wasm;
  EXPECT_EQ(wasm.read("02_test_prio1.wasm"), 0);

  std::string func = "_test_call_composition";
  ImmediateRepr result_repr;
  Immediate result;
  
  Runtime runtime(wasm);
  runtime.run(func, result_repr, result);

  EXPECT_EQ(runtime.read_memory(2, 0, ImmediateRepr::I32).n32, 35);
}
 
