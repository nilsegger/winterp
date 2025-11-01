#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include "instructions.hpp"
#include "leb128.hpp"
#include "runtime.hpp"
#include "sections.hpp"

int main(int argn, char **argc) {

  if (argn < 2) {
    std::cout << "missing input file" << std::endl;
    return 1;
  }

  WasmFile wasm;
  if(wasm.read(argc[1])) {
    return 1;
  }

  for(auto& mem: wasm.memory) {
    std::cout << "Memory " << std::hex << mem.flag << std::hex << " " << mem.n << " - " << mem.maximum << std::endl;
  }

  // Summarise Functions

  for(auto& exp : wasm.exports) {

    if(exp.kind != ExportKind::func) {
      continue;
    }
    
    std::cout << exp.name << std::endl;
    Code& f = wasm.codes[exp.idx];

    for(auto& instr: f.expr) {
      std::cout << std::hex << instr.op << "\t\t";
      for(auto& imm: instr.imms) {
        std::cout << std::hex << imm.n32 << ",\t";
      }
      std::cout<<std::endl;
    }
  }

  Runtime runtime(wasm);

  std::string func = "_test_call_add";
  ImmediateRepr result_repr;
  Immediate result;
  runtime.run(func, result_repr, result);

  return 0;
}
