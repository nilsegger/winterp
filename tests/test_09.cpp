
#include <gtest/gtest.h>

#include "runtime.hpp"
#include "sections.hpp"

class Test09 : public ::testing::Test {
protected:
  static WasmFile wasm;

  static void SetUpTestSuite() {
    EXPECT_EQ(wasm.read("09_print_hello.wasm"), 0);
  }

  static void TearDownTestSuite() {}
};

WasmFile Test09::wasm;

TEST_F(Test09, hello_world) {
  std::string func = "_start";

  Runtime runtime(wasm);
  runtime.run(func);
  Immediate nwritten = runtime.read_memory(2, 20, ImmediateRepr::I32);
  // Written 'Hello World!\n', which has 14 chars (with \0)
  EXPECT_EQ(nwritten.v.n32, 14);
}
