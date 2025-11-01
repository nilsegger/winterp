#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "instructions.hpp"
#include "runtime.hpp"

Runtime::Runtime(const struct WasmFile &wasm) : wasm(wasm) {
  memory.resize(MEMORY_PAGE_SIZE);
}

void Runtime::push_stack(const Immediate &imm) {
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

void Runtime::skip_block(const Code &c, int &pc) {

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

Immediate Runtime::handle_numeric_binop_i32(const OpCode &op,
                                            const Immediate &a,
                                            const Immediate &b) {
  Immediate result;
  result.t = ImmediateRepr::I32;
  if (op == OpCode::I32Add) {
    result.v.n32 = a.v.n32 + b.v.n32;
  } else if (op == OpCode::I32Mul) {
    result.v.n32 = a.v.n32 * b.v.n32;
  } else if (op == OpCode::I32Sub) {
    result.v.n32 = a.v.n32 - b.v.n32;
  } else if (op == OpCode::GT_S) {
    result.v.n32 =
        static_cast<int32_t>(a.v.n32) > static_cast<int32_t>(b.v.n32);
  } else if (op == OpCode::LT_S) {
    result.v.n32 =
        static_cast<int32_t>(a.v.n32) < static_cast<int32_t>(b.v.n32);
  } else {
    assert(false && "todo: invalid binop for i32");
  }

  return result;
}

Immediate Runtime::handle_numeric_unop_f32(const OpCode &op,
                                           const Immediate &a) {
  Immediate result;
  result.t = ImmediateRepr::F32;

  if (op == OpCode::F32Abs) {
    result.v.p32 = std::fabs(a.v.p32);
  } else if (op == OpCode::F32Neg) {
    result.v.p32 = -a.v.p32;
  } else if (op == OpCode::F32Sqrt) {
    result.v.p32 = std::sqrt(a.v.p32);
  } else if (op == OpCode::F32Ceil) {
    result.v.p32 = std::ceil(a.v.p32);
  } else if (op == OpCode::F32Floor) {
    result.v.p32 = std::floor(a.v.p32);
  } else if (op == OpCode::F32Trunc) {
    result.v.p32 = std::trunc(a.v.p32);
  } else if (op == OpCode::F32Nearest) {
    result.v.p32 = std::rintf(a.v.p32);
  }
  return result;
}

Immediate Runtime::handle_numeric_binop_f32(const OpCode &op,
                                            const Immediate &a,
                                            const Immediate &b) {
  Immediate result;
  result.t = ImmediateRepr::F32;
  if (op == OpCode::F32Add) {
    result.v.p32 = a.v.p32 + b.v.p32;
  } else if (op == OpCode::F32Mul) {
    result.v.p32 = a.v.p32 * b.v.p32;
  } else if (op == OpCode::F32Sub) {
    result.v.p32 = a.v.p32 - b.v.p32;
  } else if (op == OpCode::F32Div) {
    result.v.p32 = a.v.p32 / b.v.p32;
  } else if (op == OpCode::F32Min) {
    result.v.p32 = std::min(a.v.p32, b.v.p32);
  } else if (op == OpCode::F32Max) {
    result.v.p32 = std::max(a.v.p32, b.v.p32);
  } else {

    // Most likely a Comparison op
    result.t = ImmediateRepr::I32;
    if (op == OpCode::F32EQ) {
      result.v.n32 = a.v.p32 == b.v.p32;
    } else if (op == OpCode::F32Ne) {
      result.v.n32 = a.v.p32 != b.v.p32;
    }

    else if (op == OpCode::F32Lt) {
      result.v.n32 = a.v.p32 < b.v.p32;
    } else if (op == OpCode::F32Gt) {
      result.v.n32 = a.v.p32 > b.v.p32;
    }

    else if (op == OpCode::F32Le) {
      result.v.n32 = a.v.p32 <= b.v.p32;
    }

    else if (op == OpCode::F32Ge) {
      result.v.n32 = a.v.p32 >= b.v.p32;
    } else {
      assert(false && "todo: invalid binop for f32");
    }
  }
  return result;
}

Immediate Runtime::handle_numeric_unop_f64(const OpCode &op,
                                           const Immediate &a) {
  Immediate result;
  result.t = ImmediateRepr::F64;

  if (op == OpCode::F64Abs) {
    result.v.p64 = std::fabs(a.v.p64);
  } else if (op == OpCode::F64Neg) {
    result.v.p64 = -a.v.p64;
  } else if (op == OpCode::F64Sqrt) {
    result.v.p64 = std::sqrt(a.v.p64);
  } else if (op == OpCode::F64Ceil) {
    result.v.p64 = std::ceil(a.v.p64);
  } else if (op == OpCode::F64Floor) {
    result.v.p64 = std::floor(a.v.p64);
  } else if (op == OpCode::F64Trunc) {
    result.v.p64 = std::trunc(a.v.p64);
  } else if (op == OpCode::F64Nearest) {
    result.v.p64 = std::rintf(a.v.p64);
  } else {
    assert(false && "todo");
  }
  return result;
}

Immediate Runtime::handle_numeric_binop_f64(const OpCode &op,
                                            const Immediate &a,
                                            const Immediate &b) {
  Immediate result;
  result.t = ImmediateRepr::F64;
  if (op == OpCode::F64Add) {
    result.v.p64 = a.v.p64 + b.v.p64;
  } else if (op == OpCode::F64Mul) {
    result.v.p64 = a.v.p64 * b.v.p64;
  } else if (op == OpCode::F64Sub) {
    result.v.p64 = a.v.p64 - b.v.p64;
  } else if (op == OpCode::F64Div) {
    result.v.p64 = a.v.p64 / b.v.p64;
  } else if (op == OpCode::F64Min) {
    result.v.p64 = std::min(a.v.p64, b.v.p64);
  } else if (op == OpCode::F64Max) {
    result.v.p64 = std::max(a.v.p64, b.v.p64);
  } else {

    // Most likely a Comparison op
    result.t = ImmediateRepr::I64;
    if (op == OpCode::F64EQ) {
      result.v.n64 = a.v.p64 == b.v.p64;
    } else if (op == OpCode::F64Ne) {
      result.v.n64 = a.v.p64 != b.v.p64;
    }

    else if (op == OpCode::F64Lt) {
      result.v.n64 = a.v.p64 < b.v.p64;
    } else if (op == OpCode::F64Gt) {
      result.v.n64 = a.v.p64 > b.v.p64;
    }

    else if (op == OpCode::F64Le) {
      result.v.n64 = a.v.p64 <= b.v.p64;
    }

    else if (op == OpCode::F64Ge) {
      result.v.n64 = a.v.p64 >= b.v.p64;
    } else {
      assert(false && "todo: invalid binop for f64");
    }
  }
  return result;
}

Immediate Runtime::handle_conversion(const OpCode &op, const Immediate &a) {
  Immediate result;
  if (op == I32WrapI64) {
    result.v.n32 = static_cast<uint32_t>(result.v.n64);
    result.t = ImmediateRepr::I32;
  } else if (op == F32ConvertSI32) {
    result.t = ImmediateRepr::F32;
    result.v.p32 = static_cast<float>((int32_t)result.v.n32);
  } else if (op == F32ConvertUI32) {
    result.t = ImmediateRepr::F32;
    result.v.p32 = static_cast<float>(result.v.n32);
  } else if (op == F32ConvertSI64) {
    result.t = ImmediateRepr::F32;
    result.v.p32 = static_cast<float>((int64_t)result.v.n64);
  } else if (op == F32ConvertUI64) {
    result.t = ImmediateRepr::F32;
    result.v.p32 = static_cast<float>(result.v.n64);
  } else if (op == F32DemoteF64) {
    result.t = ImmediateRepr::F32;
    result.v.p32 = static_cast<float>(result.v.p64);
  } else if (op == F64ConvertSI32) {
    result.t = ImmediateRepr::F64;
    result.v.p64 = static_cast<double>((int32_t)result.v.n32);
  } else if (op == F64ConvertUI32) {
    result.t = ImmediateRepr::F64;
    result.v.p64 = static_cast<double>(result.v.n32);
  } else if (op == F64ConvertSI64) {
    result.t = ImmediateRepr::F64;
    result.v.p64 = static_cast<double>((int64_t)result.v.n64);
  } else if (op == F64ConvertUI64) {
    result.t = ImmediateRepr::F64;
    result.v.p64 = static_cast<double>(result.v.n64);
  } else if (op == F32PromoteF64) {
    result.t = ImmediateRepr::F64;
    result.v.p64 = static_cast<double>(result.v.p32);
  } else if (op == I32TruncSF32) {
    result.t = ImmediateRepr::I32;
    result.v.n32 =
        static_cast<int32_t>(static_cast<int64_t>(std::trunc(result.v.p32)));
  } else if (op == I32TruncUF32) {
    result.t = ImmediateRepr::I32;
    result.v.n32 =
        static_cast<uint32_t>(static_cast<uint64_t>(std::trunc(result.v.p32)));
  } else if (op == I32TruncSF64) {
    result.t = ImmediateRepr::I32;
    result.v.n32 =
        static_cast<int32_t>(static_cast<int64_t>(std::trunc(result.v.p64)));
  } else if (op == I32TruncUF64) {
    result.t = ImmediateRepr::I32;
    result.v.n32 =
        static_cast<uint32_t>(static_cast<uint64_t>(std::trunc(result.v.p64)));
  } else if (op == I64ExtendSI32) {
    result.t = ImmediateRepr::I64;
    result.v.n64 = static_cast<int64_t>(static_cast<int32_t>(result.v.n32));
  } else if (op == I64ExtendUI32) {
    result.t = ImmediateRepr::I64;
    result.v.n64 = static_cast<uint64_t>(static_cast<uint32_t>(result.v.n32));
  } else if (op == I64TruncSF32) {
    result.t = ImmediateRepr::I64;
    result.v.n64 = static_cast<int64_t>(std::trunc(result.v.p32));
  } else if (op == I64TruncUF32) {
    result.t = ImmediateRepr::I64;
    result.v.n64 = static_cast<uint64_t>(std::trunc(result.v.p32));
  } else if (op == I64TruncSF64) {
    result.t = ImmediateRepr::I64;
    result.v.n64 = static_cast<int64_t>(std::trunc(result.v.p64));
  } else if (op == I64TruncUF64) {
    result.t = ImmediateRepr::I64;
    result.v.n64 = static_cast<uint64_t>(std::trunc(result.v.p64));
  } else {
    assert(false && "todo: missing case");
  }
  return result;
}

Immediate Runtime::reinterp(const Immediate &a, const ImmediateRepr from,
                            const ImmediateRepr to) {
  assert(a.t == from && "invalid reinterpretation?");
  Immediate c1 = a;
  c1.t = to;
  return c1;
}

void Runtime::execute(int function_index) {

  std::cout << "Function " << std::dec << function_index << " called"
            << std::endl;
  std::cout << "Stack size " << this->stack.size() << std::endl;

  // Again, assumes all indices are valid...
  const Code &block = wasm.codes[function_index];

  std::cout << "Function has " << block.locals.size() << " locals."
            << std::endl;

  typeidx function_signature_index = wasm.function_section[function_index];

  std::cout << "Function signature index is " << function_signature_index
            << std::endl;

  // important for stack information
  const FunctionType &signature = wasm.type_section[function_signature_index];

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

    const Instr &instr = block.expr[pc];

    uint8_t op_byte = static_cast<uint8_t>(instr.op);

    bool is_i32_numeric_binop = (op_byte >= 0x6A && op_byte <= 0x78) ||
                                (op_byte >= 0x45 && op_byte <= 0x4F);
    bool is_f32_numeric_unop = op_byte >= 0x8B && op_byte <= 0x91;
    bool is_f32_numeric_binop = (op_byte >= 0x92 && op_byte <= 0x98) ||
                                (op_byte >= 0x5B && op_byte <= 0x60);

    bool is_f64_numeric_unop = (op_byte >= 0x99 && op_byte <= 0x9F);
    bool is_f64_numeric_binop = (op_byte >= 0xA0 && op_byte <= 0xA5) ||
                                (op_byte >= 0x61 && op_byte <= 0x66);

    std::cout << "Read OP " << std::hex << instr.op << std::dec << std::endl;

    // Instructions implemented based on description here
    // https://webassembly.github.io/spec/core/exec/instructions.html

    if (instr.op == OpCode::End) {
      // Treat as nop, only last End actually ends the function, the rest are
      // useful to know when control blocks end
    } else if (instr.op == OpCode::Else) {
      // We have landed in a Else block, which we do not want to execute
      // We know that we can skip this block, because if the if block would have
      // taken the else route, skip_block would have stoped at the first op to
      // actually execute after the else
      std::cout << "Skipping else block!" << std::endl;
      skip_block(block, pc);
      continue;
    } else if (instr.op == OpCode::Call) {
      execute(instr.imms[0].v.n32);
    } else if (instr.op == OpCode::I32Const || instr.op == OpCode::F32Const ||
               instr.op == OpCode::I64Const || instr.op == OpCode::F64Const) {
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
    /* Convertion, Promotion, Demotion */
    else if (op_byte >= 0xA7 && op_byte <= 0xBB) {
      Immediate a = this->pop_stack();
      this->push_stack(handle_conversion(instr.op, a));
    }
    /* UNOP NUMERIC INSTRUCTIONS */
    else if (is_f32_numeric_unop || is_f64_numeric_unop) {
      Immediate a = this->pop_stack();
      Immediate b;
      if (is_f32_numeric_unop) {
        b = handle_numeric_unop_f32(instr.op, a);
      } else if (is_f64_numeric_unop) {
        b = handle_numeric_unop_f64(instr.op, a);
      } else {
        assert(false && "todo!");
      }
      this->push_stack(b);
    }
    /* BINOP NUMERIC INSTRUCTIONS */
    else if (is_i32_numeric_binop || is_f32_numeric_binop ||
             is_f64_numeric_binop) {
      Immediate b = this->pop_stack();
      Immediate a = this->pop_stack();

      Immediate c;

      if (is_i32_numeric_binop) {
        c = handle_numeric_binop_i32(instr.op, a, b);
      } else if (is_f32_numeric_binop) {
        c = handle_numeric_binop_f32(instr.op, a, b);
      } else if (is_f64_numeric_binop) {
        c = handle_numeric_binop_f64(instr.op, a, b);
      } else {
        assert(false && "todo!");
      }

      this->push_stack(c);
    }
    /* REINTERP */
    else if (op_byte >= 0xBC && op_byte <= 0xBF) {
      Immediate a = this->pop_stack();
      Immediate result;
      if (instr.op == I32ReinterpF32) {
        result = reinterp(a, ImmediateRepr::F32, ImmediateRepr::I32);
      } else if (instr.op == F32ReinterpI32) {
        result = reinterp(a, ImmediateRepr::I32, ImmediateRepr::F32);
      } else if (instr.op == I64ReinterpF64) {
        result = reinterp(a, ImmediateRepr::F64, ImmediateRepr::I64);
      } else if (instr.op == F64ReinterpI64) {
        result = reinterp(a, ImmediateRepr::I64, ImmediateRepr::F64);
      }
      this->push_stack(result);
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
