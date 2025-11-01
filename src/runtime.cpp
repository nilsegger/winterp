#include <cassert>
#include <cstring>
#include <iostream>

#include "instructions.hpp"
#include "runtime.hpp"

Runtime::Runtime(struct WasmFile &wasm) : wasm(wasm) {
  memory.resize(MEMORY_PAGE_SIZE);
}

void Runtime::push_stack(Immediate &imm) {
  assert(imm.t != ImmediateRepr::Uninitialised);
 this->stack.push_back(imm);
}

Immediate Runtime::pop_stack() {
  if (this->stack.size() >= 1) {
    Immediate value = this->stack.back();
    assert(value.t != ImmediateRepr::Uninitialised);
    this->stack.pop_back();
    return value;
  } else {
    assert(false && "malformed stack size.");
  }
}

void Runtime::write_memory(const uint32_t &mem_index, const uint32_t &offset,
                           const Immediate &imm) {
  // TODO: actual store with mem_index

  std::cout << "WRITE TO MEMORY " << std::dec << mem_index << " offset "
            << offset << std::endl;
  std::cout << "Type " << std::hex << imm.t << std::endl;
  std::cout << "Value " << std::dec << imm.v.n32 << std::endl;

  switch (imm.t) {
  case ImmediateRepr::Uninitialised:
    assert(false && "Invalid repr found.");
    break;
  case ImmediateRepr::I32:
  case ImmediateRepr::F32:
    assert(offset + 4 < this->memory.size() && "invalid memory access");
    std::memcpy(&this->memory[offset], &imm.v.n32, 4);
    break;
  case ImmediateRepr::I64:
  case ImmediateRepr::F64:
    assert(offset + 8 < this->memory.size() && "invalid memory access");
    std::memcpy(&this->memory[offset], &imm.v.n64, 8);
    break;
  default:
    assert(false && "todo");
  }
}

Immediate Runtime::read_memory(const uint32_t &mem_index,
                               const uint32_t &offset,
                               const ImmediateRepr repr) {

  Immediate read;
  read.t = repr;

  // TODO: actual store with mem_index
  switch (repr) {
  case ImmediateRepr::Uninitialised:
    assert(false && "Invalid repr found.");
    break;
  case ImmediateRepr::I32:
    assert(offset + 4 < this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.n32, &this->memory[offset], 4);
    break;
  case ImmediateRepr::F32:
    assert(offset + 4 < this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.p32, &this->memory[offset], 4);
    break;
  case ImmediateRepr::I64:
    assert(offset + 8 < this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.n64, &this->memory[offset], 8);
    break;
  case ImmediateRepr::F64:
    assert(offset + 8 < this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.p64, &this->memory[offset], 8);
    break;
  default:
    assert(false && "todo");
  }

  return read;
}

void Runtime::execute(int function_index) {

  std::cout << "Function " << std::dec << function_index << " called"
            << std::endl;
  std::cout << "Stack size " << this->stack.size() << std::endl;

  // Again, assumes all indices are valid...
  Code &c = wasm.codes[function_index];

  std::cout << "Function has " << c.locals.size() << " locals." << std::endl;

  typeidx function_signature_index = wasm.function_section[function_index];

  // important for stack information
  FunctionType &signature = wasm.type_section[function_signature_index];

  std::cout << "Function has " << signature.params.size() << " params"
            << std::endl;

  /* TODO: pop stack, what about locals? */
  std::vector<Immediate> params(signature.params.size());
  for (int i = 0; i < signature.params.size(); i++) {
    params[i] = this->pop_stack();
  }

  /* Emulate Instructions */

  for (int i = 0; i < c.expr.size(); i++) {
    Instr &instr = c.expr[i];

    // Instructions implemented based on description here
    // https://webassembly.github.io/spec/core/exec/instructions.html
    if (instr.op == OpCode::Call) {
      execute(instr.imms[0].v.n32);
    } else if (instr.op == OpCode::I32Const) {
      this->push_stack(instr.imms[0]);
    }

    /* VARIABLE INSTRUCTIONS */
    else if (instr.op == OpCode::LocalGet) {
      // The parameters of the function are referenced through 0-based local
      // indices in the function’s body; they are mutable.

      uint32_t index = instr.imms[0].v.n32;

      if (index < params.size()) {
        this->push_stack(params[index]);
      } else {
        assert(false && "working on this!");
        // index = index - params.size();
        // this->push_stack(c.locals[index]);
      }

    } else if (instr.op == OpCode::LocalSet) {
      Immediate val = this->pop_stack();
      std::cout << val.v.n32 << " value to put at " << instr.imms[0].v.n32
                << std::endl;
    }
    /* NUMERIC INSTRUCTIONS */
    else if (instr.op == OpCode::I32Add || instr.op == OpCode::I32Mul) {
      Immediate b = this->pop_stack();
      Immediate a = this->pop_stack();

      Immediate c;
      c.t = ImmediateRepr::I32;

      if (instr.op == OpCode::I32Add) {
        c.v.n32 = a.v.n32 + b.v.n32;
      } else if (instr.op == OpCode::I32Mul) {
        std::cout << a.v.n32 << " * " << b.v.n32 << std::endl;
        c.v.n32 = a.v.n32 * b.v.n32;
      }

      this->push_stack(c);
    } else if (instr.op == OpCode::I32Store) {

      if (instr.imms.size() > 2) {
        assert(false &&
               "todo: there are 3 immediates to I32Store, special case.");
      }

      Immediate x = instr.imms[0];
      Immediate ao = instr.imms[1];

      Immediate c = this->pop_stack();
      Immediate i = this->pop_stack();

      uint32_t offset = ao.v.n32 + i.v.n32;
      this->write_memory(x.v.n32, offset, c);

    } else {
      std::cout << "Missing opcode handle for " << std::hex << instr.op
                << std::endl;
      assert(false && "todo: implement new opcode emulation");
    }
  }
}

void Runtime::run(std::string &function) {

  // Lookup function in exports by string,
  // a HashMap could be more efficient as a loop, but this depends on how many
  // function entries there are this would need to be benchmarked

  int export_index = -1;

  for (int i = 0; i < this->wasm.exports.size(); i++) {
    if (this->wasm.exports[i].name == function) {
      export_index = i;
      break;
    }
  }

  assert(export_index >= 0 && "export function not found!");

  // assumes wasm is well structured with indices, otherwise might crash
  // Code c contains the assembly of the requested function to run
  int function_index = this->wasm.exports[export_index].idx;
  this->execute(function_index);
}
