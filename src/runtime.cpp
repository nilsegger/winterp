#include <cassert>
#include <cstdint>
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

// Moves the program counter, until the next else / end is found, ignoring
// nested conditions
// The program counter will be left at the first instruction to be executed.
void skip_block(const Code &c, int &pc) {

  // Assumes skip_block was called directly while pc is still one the if
  pc++;

  int conditional_nesting = 0;

  while ((c.expr[pc].op != OpCode::End && c.expr[pc].op != OpCode::Else) ||
         conditional_nesting > 0) {

    std::cout << std::dec << "PC " << pc << " : " << std::hex << c.expr[pc].op
              << std::endl;

    uint8_t op = static_cast<uint8_t>(c.expr[pc].op);
    if (0x02 <= op && op <= 0x04) {
      // Not inclusive 0x05, because else does not continue the nesting
      conditional_nesting++;
    }

    if (c.expr[pc].op == OpCode::Else) {
      assert(false && "todo: should this also conditiona_nesting--?");
    }

    if (c.expr[pc].op == OpCode::End) {
      conditional_nesting--;
    }

    pc++;
  }

  pc++;
}

void Runtime::execute(int function_index) {

  std::cout << "Function " << std::dec << function_index << " called"
            << std::endl;
  std::cout << "Stack size " << this->stack.size() << std::endl;

  // Again, assumes all indices are valid...
  Code &block = wasm.codes[function_index];

  std::cout << "Function has " << block.locals.size() << " locals."
            << std::endl;

  typeidx function_signature_index = wasm.function_section[function_index];

  std::cout << "Function signature index is " << function_signature_index
            << std::endl;

  // important for stack information
  FunctionType &signature = wasm.type_section[function_signature_index];

  std::cout << "Function has " << signature.params.size() << " params"
            << std::endl;

  /* Pop Stack based on signature params */
  std::vector<Immediate> params(signature.params.size());
  for (int i = 0; i < signature.params.size(); i++) {
    params[i] = this->pop_stack();
  }

  /* Prepare Locals */
  std::vector<Immediate> locals;
  for (auto &local : block.locals) {
    for (int j = 0; j < local.count; j++) {
      Immediate l;
      l.t = local.type;
      l.v.n64 = 0;
      locals.push_back(l);
    }
  }

  /* Emulate Instructions */
  // program counter, which instruction were currently running
  int pc = 0;
  while (pc < block.expr.size()) {
   
    Instr &instr = block.expr[pc];

    std::cout << "Read OP " << std::hex << instr.op << std::dec << std::endl;

    // Instructions implemented based on description here
    // https://webassembly.github.io/spec/core/exec/instructions.html

    if (instr.op == OpCode::End) {
      // Treat as nop, only last End actually ends the function, the rest are
      // useful to know when control blocks end
    } else if(instr.op == OpCode::Else) {
       // We have landed in a Else block, which we do not want to execute 
       // We know that we can skip this block, because if the if block would have taken the else route,
       // skip_block would have stoped at the first op to actually execute after the else
       std::cout << "Skipping else block!" << std::endl;
       skip_block(block, pc);
       continue;
    } else if (instr.op == OpCode::Call) {
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
        index = index - params.size();
        assert(index < locals.size() && "LocalGet invalid local index!");
        Immediate v = locals[index];
        std::cout << "read local index " << index << " with value " << v.v.n32
                  << std::endl;
        this->push_stack(locals[index]);
      }

    } else if (instr.op == OpCode::LocalSet) {
      Immediate val = this->pop_stack();

      uint32_t index = instr.imms[0].v.n32;
      if (index < params.size()) {
        params[index] = val;
      } else {
        index = index - params.size();
        assert(index < locals.size() && " LocalSet invalid local index!");
        locals[index] = val;
      }
    }
    /* NUMERIC INSTRUCTIONS */
    else if (instr.op == OpCode::I32Add || instr.op == OpCode::I32Sub ||
             instr.op == OpCode::I32Mul || instr.op == OpCode::GT_S ||
             instr.op == OpCode::LT_S) {
      Immediate b = this->pop_stack();
      Immediate a = this->pop_stack();

      Immediate c;
      c.t = ImmediateRepr::I32;

      if (instr.op == OpCode::I32Add) {
        c.v.n32 = a.v.n32 + b.v.n32;
      } else if (instr.op == OpCode::I32Mul) {
        c.v.n32 = a.v.n32 * b.v.n32;
      } else if (instr.op == OpCode::I32Sub) {
        c.v.n32 = a.v.n32 - b.v.n32;
      } else if (instr.op == OpCode::GT_S) {
        // TODO: signed?
        c.v.n32 = static_cast<int32_t>(a.v.n32) > static_cast<int32_t>(b.v.n32);
      } else if (instr.op == OpCode::LT_S) {
        c.v.n32 = static_cast<int32_t>(a.v.n32) < static_cast<int32_t>(b.v.n32);
      } else {
        assert(false && "todo");
      }

      this->push_stack(c);
    } else if (instr.op == OpCode::If) {
      Immediate c = this->pop_stack();
      if (c.v.n32) {
        // execute first block
      } else {
        std::cout << "skipping !" << std::endl;
        // execute second block
        // find Else statement or end statement, ignore else/end statements of
        // nested ifs!
        skip_block(block, pc);
        // dont include pc++ below, this would skip the next meaningfull op
        continue;
      }
    } else if (instr.op == OpCode::Return) {
      // TODO: read call frame from stack, return to last caller
      break;
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

    pc++;
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
