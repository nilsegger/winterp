#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "bits.hpp"
#include "instructions.hpp"
#include "sections.hpp"
#include "runtime.hpp"

Runtime::Runtime(const struct WasmFile &wasm) : wasm(wasm) {
  // TODO: instantiate memory from wasm.memory
  memory.resize(MEMORY_PAGE_SIZE);
  std::fill(memory.begin(), memory.end(), 0);
  pages = 1;

  // Reserve memory of table, also verify only supported reftype is used
  for (const auto &table : wasm.tables) {
    assert(table.ref_type == 0x70 &&
           "todo: currently only support function table, but different type "
           "was requested.");
    this->function_table.resize(std::max(table.limit_n, table.limit_m));
  }

  // Put function indices into function table
  for (const auto &elem : wasm.elems) {
    /* Evaluate expression to know offset of function index */

    this->execute_block(elem.expr);
    Immediate offset = this->pop_stack();
    assert(offset.t == ImmediateRepr::I32 && "todo: wrong repr assumed.");

    for (int i = 0; i < elem.function_indices.size(); i++) {
      uint32_t entry_index = offset.v.n32 + i;
      this->function_table[entry_index] = elem.function_indices[i];
    }
  }

  // Put initial data into memory
  for (const auto &data : wasm.data) {
    this->execute_block(data.expr);
    Immediate offset = this->pop_stack();
    assert(offset.t == ImmediateRepr::I32 && "todo: wrong repr assumed.");

    std::memcpy(&this->memory[offset.v.n32], data.bytes.data(),
                data.bytes.size());
  }

  // Setup Globals
  for (const auto &global : wasm.globals) {
    this->execute_block(global.expr);
    GlobalInstance instance;
    instance.mut = global.mutability;
    instance.value = this->pop_stack();
    this->globals.push_back(instance);
  }

  // Setup data segments
  this->data = wasm.data;
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
  case ImmediateRepr::Byte:
    assert(offset + 1 <= this->memory.size() && "invalid memory access");
    read.v.n32 = static_cast<uint32_t>(this->memory[offset]);
    break;
  case ImmediateRepr::I32:
    assert(offset + 4 <= this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.n32, &this->memory[offset], 4);
    break;
  case ImmediateRepr::F32:
    assert(offset + 4 <= this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.p32, &this->memory[offset], 4);
    break;
  case ImmediateRepr::I64:
    assert(offset + 8 <= this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.n64, &this->memory[offset], 8);
    break;
  case ImmediateRepr::F64:
    assert(offset + 8 <= this->memory.size() && "invalid memory access");
    std::memcpy(&read.v.p64, &this->memory[offset], 8);
    break;
  default:
    assert(false && "todo");
  }

  return read;
}

void Runtime::skip_control_block(const std::vector<Instr> &block, int &pc) {

  // Assumes skip_block was called directly while pc is still one the if
  pc++;

  int conditional_nesting = 0;

  while ((block[pc].op != OpCode::End && block[pc].op != OpCode::Else) ||
         conditional_nesting > 0) {

    uint8_t op = static_cast<uint8_t>(block[pc].op);
    if (0x02 <= op && op <= 0x04) {
      // Not inclusive 0x05, because else does not continue the nesting, instead
      // is must already have been added by its corresponding if
      conditional_nesting++;
    }

    // This case can happen, after block_branch was called, and else is
    // immediatly after a br.
    if (block[pc].op == OpCode::Else) {
      conditional_nesting--;
    }

    if (block[pc].op == OpCode::End) {
      conditional_nesting--;
    }

    pc++;
  }

  pc++;
}

void Runtime::branch_block(const std::vector<Instr> &block, int &pc,
                           int label) {

  // Branch block behaves differently for loops and blocks
  // For blocks, pc is set to after the block
  // while for loop pc it is set to the beginning of the loop!
  // Label tells us how many blocks / loops were trying to escape out of, in
  // this case its not really a label, more a counter of how many blocks/loops
  // were trying to escape out of

  // label = 0
  // -> in a block, go to the end of the block
  // -> in a loop, go to the start of the loop

  // Need to increase by one since it start at 0
  label++;

  int conditional_nesting = 0;

  while (label > 0) {

    uint8_t op = static_cast<uint8_t>(block[pc].op);

    // Found a block / loop
    if ((op == 0x02 || op == 0x03) && conditional_nesting == 0) {
      label--;
    } else if ((op == 0x02 || op == 0x03)) {
      conditional_nesting--;
    }

    // We are moving up and have found an end, hence the above must be nested!
    if (op == 0x0b) {
      conditional_nesting++;
    }

    if (label > 0) {
      pc--;
    }
  }

  if (block[pc].op == OpCode::Block) {
    // We are branching to the end
    skip_control_block(block, pc);
  } else if (block[pc].op == OpCode::Loop) {
    // Looping! We are staying but advancing by once
    pc++;
  } else {
    assert(false && "Branching stopped at an invalid op code.");
  }

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
  } else if (op == OpCode::I32DivS) {
    assert(b.v.n32 != 0 && "division by 0");
    result.v.n32 =
        static_cast<int32_t>(a.v.n32) / static_cast<int32_t>(b.v.n32);
  } else if (op == OpCode::I32DivU) {
    assert(b.v.n32 != 0 && "division by 0");
    result.v.n32 = a.v.n32 / b.v.n32;
  } else if (op == OpCode::I32RemS) {
    assert(b.v.n32 != 0 && "division by 0");
    result.v.n32 =
        static_cast<int32_t>(a.v.n32) % static_cast<int32_t>(b.v.n32);
  } else if (op == OpCode::I32RemU) {
    assert(b.v.n32 != 0 && "division by 0");
    result.v.n32 = a.v.n32 % b.v.n32;
  } else if (op == OpCode::I32eq) {
    result.v.n32 = (a.v.n32 == b.v.n32);
  } else if (op == OpCode::I32ne) {
    result.v.n32 = (a.v.n32 != b.v.n32);
  } else if (op == OpCode::I32ltu) {
    result.v.n32 = a.v.n32 < b.v.n32;
  } else if (op == OpCode::I32lts) {
    result.v.n32 =
        (static_cast<int32_t>(a.v.n32) < static_cast<int32_t>(b.v.n32));
  } else if (op == OpCode::I32gtu) {
    result.v.n32 = a.v.n32 > b.v.n32;
  } else if (op == OpCode::I32gts) {
    result.v.n32 =
        (static_cast<int32_t>(a.v.n32) > static_cast<int32_t>(b.v.n32));
  } else if (op == OpCode::I32le_s) {
    result.v.n32 =
        (static_cast<int32_t>(a.v.n32) <= static_cast<int32_t>(b.v.n32));
  } else if (op == OpCode::I32le_u) {
    result.v.n32 = a.v.n32 <= b.v.n32;
  } else if (op == OpCode::I32ge_s) {
    result.v.n32 =
        (static_cast<int32_t>(a.v.n32) >= static_cast<int32_t>(b.v.n32));
  } else if (op == OpCode::I32ge_u) {
    result.v.n32 = a.v.n32 >= b.v.n32;
  } else if (op == OpCode::I32and) {
    result.v.n32 = a.v.n32 & b.v.n32;
  } else if (op == OpCode::I32or) {
    result.v.n32 = a.v.n32 | b.v.n32;
  } else if (op == OpCode::I32xor) {
    result.v.n32 = a.v.n32 ^ b.v.n32;
  } else if (op == OpCode::I32shl) {
    result.v.n32 = a.v.n32 << b.v.n32;
  } else if (op == OpCode::I32shrs) {
    result.v.n32 =
        static_cast<int32_t>(a.v.n32) >> static_cast<int32_t>(b.v.n32);
  } else if (op == OpCode::I32shru) {
    result.v.n32 = a.v.n32 >> b.v.n32;
  } else if (op == OpCode::I32rotl) {
    uint32_t shift = b.v.n32 & 0x3F;
    result.v.n32 = (a.v.n32 << shift) | (a.v.n32 >> (32 - shift));
  } else if (op == OpCode::I32rotr) {
    uint32_t shift = b.v.n32 & 0x3F;
    result.v.n32 = (a.v.n32 >> shift) | (a.v.n32 << (32 - shift));
  } else {
    assert(false && "todo: invalid binop for i32");
  }

  return result;
}

Immediate Runtime::handle_numeric_unop_i32(const OpCode &op,
                                           const Immediate &a) {

  Immediate result;
  result.t = ImmediateRepr::I32;

  if (op == I32eqz) {
    result.v.n32 = a.v.n32 == 0;
  } else if (op == OpCode::I32clz) {
    result.v.n32 = clz(a.v.n32);
  } else if (op == OpCode::I32ctz) {
    result.v.n32 = ctz(a.v.n32);
  } else if (op == OpCode::I32popcnt) {
    result.v.n32 = popcnt(a.v.n32);
  } else {
    assert(false && "todo: missing opcode");
  }

  return result;
}

Immediate Runtime::handle_numeric_unop_i64(const OpCode &op,
                                           const Immediate &a) {

  Immediate result;
  result.t = ImmediateRepr::I64;

  if (op == I64eqz) {
    result.v.n64 = a.v.n64 == 0;
  } else if (op == OpCode::I64clz) {
    result.v.n64 = clz(a.v.n64);
  } else if (op == OpCode::I64ctz) {
    result.v.n64 = ctz(a.v.n64);
  } else if (op == OpCode::I64popcnt) {
    result.v.n64 = popcnt(a.v.n64);
  } else {
    assert(false && "todo: missing opcode");
  }

  return result;
}

Immediate Runtime::handle_numeric_binop_i64(const OpCode &op,
                                            const Immediate &a,
                                            const Immediate &b) {
  Immediate result;
  result.t = ImmediateRepr::I64;
  if (op == OpCode::I64Add) {
    result.v.n64 = a.v.n64 + b.v.n64;
  } else if (op == OpCode::I64Mul) {
    result.v.n64 = a.v.n64 * b.v.n64;
  } else if (op == OpCode::I64Sub) {
    result.v.n64 = a.v.n64 - b.v.n64;
  } else if (op == OpCode::I64DivS) {
    assert(b.v.n64 != 0 && "division by 0");
    result.v.n64 =
        static_cast<int64_t>(a.v.n64) / static_cast<int64_t>(b.v.n64);
  } else if (op == OpCode::I64DivU) {
    assert(b.v.n64 != 0 && "division by 0");
    result.v.n64 = a.v.n64 / b.v.n64;
  } else if (op == OpCode::I64RemS) {
    assert(b.v.n64 != 0 && "division by 0");
    result.v.n64 =
        static_cast<int64_t>(a.v.n64) % static_cast<int64_t>(b.v.n64);
  } else if (op == OpCode::I64RemU) {
    assert(b.v.n64 != 0 && "division by 0");
    result.v.n64 = a.v.n64 % b.v.n64;
  } else if (op == OpCode::I64and) {
    result.v.n64 = a.v.n64 & b.v.n64;
  } else if (op == OpCode::I64or) {
    result.v.n64 = a.v.n64 | b.v.n64;
  } else if (op == OpCode::I64xor) {
    result.v.n64 = a.v.n64 ^ b.v.n64;
  } else if (op == OpCode::I64shl) {
    result.v.n64 = a.v.n64 << b.v.n64;
  } else if (op == OpCode::I64shrs) {
    result.v.n64 =
        static_cast<uint64_t>(a.v.n64) >> static_cast<uint64_t>(b.v.n64);
  } else if (op == OpCode::I64shru) {
    result.v.n64 = a.v.n64 >> b.v.n64;
  } else if (op == OpCode::I64rotl) {
    uint64_t shift = b.v.n64 & 0x3F;
    result.v.n64 = (a.v.n64 << shift) | (a.v.n64 >> (64 - shift));
  } else if (op == OpCode::I64rotr) {
    uint64_t shift = b.v.n64 & 0x3F;
    result.v.n64 = (a.v.n64 >> shift) | (a.v.n64 << (64 - shift));
  } else if (op == OpCode::I64eqz) {
    result.v.n32 = (a.v.n64 == 0) ? 1 : 0;
  } else if (op == OpCode::I64eq) {
    result.v.n32 = (a.v.n64 == b.v.n64) ? 1 : 0;
  } else if (op == OpCode::I64ne) {
    result.v.n32 = (a.v.n64 != b.v.n64) ? 1 : 0;
  } else if (op == OpCode::I64lts) {
    result.v.n32 =
        (static_cast<int64_t>(a.v.n64) < static_cast<int64_t>(b.v.n64)) ? 1 : 0;
  } else if (op == OpCode::I64gts) {
    result.v.n32 =
        (static_cast<int64_t>(a.v.n64) > static_cast<int64_t>(b.v.n64)) ? 1 : 0;
  } else if (op == OpCode::I64gts) {
    result.v.n32 =
        (static_cast<int64_t>(a.v.n64) > static_cast<int64_t>(b.v.n64)) ? 1 : 0;
  } else if (op == OpCode::I64gtu) {
    result.v.n32 = a.v.n64 > b.v.n64 ? 1 : 0;
  } else if (op == OpCode::I64les) {
    result.v.n32 =
        (static_cast<int64_t>(a.v.n64) <= static_cast<int64_t>(b.v.n64)) ? 1
                                                                         : 0;
  } else if (op == OpCode::I64leu) {
    result.v.n32 = a.v.n64 <= b.v.n64 ? 1 : 0;
  } else if (op == OpCode::I64ges) {
    result.v.n32 =
        (static_cast<int64_t>(a.v.n64) >= static_cast<int64_t>(b.v.n64)) ? 1
                                                                         : 0;
  } else if (op == OpCode::I64geu) {
    result.v.n32 = a.v.n64 >= b.v.n64 ? 1 : 0;
  } else {
    assert(false && "todo: invalid binop for i64");
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
  } else {
    assert(false && "todo: invalid f32 binop");
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
  } else if (op == OpCode::F32CopySign) {
    result.v.p32 = std::copysign(a.v.p32, b.v.p32);
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
  } else if (op == OpCode::F64CopySign) {
    result.v.p64 = std::copysign(a.v.p64, b.v.p64);
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

Immediate Runtime::handle_load(const OpCode &op, const uint32_t &mem_index,
                               const uint32_t &offset) {

  Immediate result;
  if (op == OpCode::I32Load) {
    return this->read_memory(mem_index, offset, ImmediateRepr::I32);
  } else if (op == OpCode::I64Load) {
    return this->read_memory(mem_index, offset, ImmediateRepr::I64);
  } else if (op == OpCode::F32Load) {
    return this->read_memory(mem_index, offset, ImmediateRepr::F32);
  } else if (op == OpCode::F64Load) {
    return this->read_memory(mem_index, offset, ImmediateRepr::F64);
  } else if (op == OpCode::I32Load8S) {
    result.t = ImmediateRepr::I32;
    int8_t data;
    std::memcpy(&data, &this->memory[offset], 1);
    result.v.n32 = static_cast<int32_t>(data);
  }

  else if (op == OpCode::I32Load8U) {
    result.t = ImmediateRepr::I32;
    uint8_t data;
    std::memcpy(&data, &this->memory[offset], 1);
    result.v.n32 = static_cast<uint32_t>(data);
  }

  else if (op == OpCode::I32Load16S) {
    result.t = ImmediateRepr::I32;
    int16_t data;
    std::memcpy(&data, &this->memory[offset], 2);
    result.v.n32 = static_cast<int32_t>(data);
  }

  else if (op == OpCode::I32Load16U) {
    result.t = ImmediateRepr::I32;
    uint16_t data;
    std::memcpy(&data, &this->memory[offset], 2);
    result.v.n32 = static_cast<uint32_t>(data);
  }

  else if (op == OpCode::I64Load8S) {
    result.t = ImmediateRepr::I64;
    int8_t data;
    std::memcpy(&data, &this->memory[offset], 1);
    result.v.n64 = static_cast<int64_t>(data);
  }

  else if (op == OpCode::I64Load8U) {
    result.t = ImmediateRepr::I64;
    uint8_t data;
    std::memcpy(&data, &this->memory[offset], 1);
    result.v.n64 = static_cast<uint64_t>(data);
  }

  else if (op == OpCode::I64Load16S) {
    result.t = ImmediateRepr::I64;
    int16_t data;
    std::memcpy(&data, &this->memory[offset], 2);
    result.v.n64 = static_cast<int64_t>(data);
  }

  else if (op == OpCode::I64Load16U) {
    result.t = ImmediateRepr::I64;
    uint16_t data;
    std::memcpy(&data, &this->memory[offset], 2);
    result.v.n64 = static_cast<uint64_t>(data);
  }

  else if (op == OpCode::I64Load32S) {
    result.t = ImmediateRepr::I64;
    int32_t data;
    std::memcpy(&data, &this->memory[offset], 4);
    result.v.n64 = static_cast<int64_t>(data);
  }

  else if (op == OpCode::I64Load32U) {
    result.t = ImmediateRepr::I64;
    uint32_t data;
    std::memcpy(&data, &this->memory[offset], 4);
    result.v.n64 = static_cast<uint64_t>(data);
  } else {
    assert(false && "todo: missing case");
  }

  return result;
}

void Runtime::handle_store(const OpCode &op, const uint32_t &mem_index,
                           const uint32_t &offset, const Immediate &value) {

  uint8_t op_byte = static_cast<uint8_t>(op);

  if (op_byte >= 0x36 && op_byte <= 0x39) {
    this->write_memory(mem_index, offset, value);
  }
  // TODO: bounds checking...
  else if (op == I32Store8) {
    uint8_t byte = static_cast<uint8_t>(value.v.n32 & 0xFF);
    memory[offset] = byte;
  } else if (op == I32Store16) {
    std::memcpy(&this->memory[offset], &value.v.n32, 2);
  } else if (op == I64Store8) {
    uint8_t byte = static_cast<uint8_t>(value.v.n64 & 0xFF);
    memory[offset] = byte;
  } else if (op == I64Store16) {
    std::memcpy(&this->memory[offset], &value.v.n64, 2);
  } else if (op == I64Store32) {
    std::memcpy(&this->memory[offset], &value.v.n64, 4);
  } else {
    assert(false && "invalid op!");
  }
}

Immediate Runtime::reinterp(const Immediate &a, const ImmediateRepr from,
                            const ImmediateRepr to) {
  assert(a.t == from && "invalid reinterpretation?");
  Immediate c1 = a;
  c1.t = to;
  return c1;
}

void Runtime::execute_block(const std::vector<Instr> &block,
                            std::vector<Immediate> &params,
                            std::vector<Immediate> &locals) {

  assert(block.size() > 0 && "execute block was called on empty expr");

  /* Emulate Instructions */
  // program counter, which instruction were currently running
  int pc = 0;
  while (pc < block.size()) {

    const Instr &instr = block[pc];

    uint8_t op_byte = static_cast<uint8_t>(instr.op);

    bool is_i32_numeric_unop =
        op_byte == 0x45 || (op_byte >= 0x67 && op_byte <= 0x69);

    bool is_i32_numeric_binop = (op_byte >= 0x6A && op_byte <= 0x78) ||
                                (op_byte >= 0x46 && op_byte <= 0x4F);

    bool is_i64_numeric_unop = (op_byte >= 0x79 && op_byte <= 0x7B) ||
                               (op_byte >= 0x50 && op_byte <= 0x50);
    bool is_i64_numeric_binop = (op_byte >= 0x7C && op_byte <= 0x8A) ||
                                (op_byte >= 0x51 && op_byte <= 0x5A);

    bool is_f32_numeric_unop = op_byte >= 0x8B && op_byte <= 0x91;
    bool is_f32_numeric_binop = (op_byte >= 0x92 && op_byte <= 0x98) ||
                                (op_byte >= 0x5B && op_byte <= 0x60) ||
                                (op_byte == 0x98);

    bool is_f64_numeric_unop = (op_byte >= 0x99 && op_byte <= 0x9F);
    bool is_f64_numeric_binop = (op_byte >= 0xA0 && op_byte <= 0xA6) ||
                                (op_byte >= 0x61 && op_byte <= 0x66);

    // Instructions implemented based on description here
    // https://webassembly.github.io/spec/core/exec/instructions.html

    assert(instr.op != OpCode::Unreachable &&
           "Unreachable statement has been hit!");

    if (instr.op == OpCode::End || instr.op == OpCode::Nop) {
      // Treat as nop, only last End actually ends the function, the rest are
      // useful to know when control blocks end
    } else if (instr.op == OpCode::Drop) {
      this->pop_stack();
    } else if (instr.op == OpCode::Select) {
      Immediate c = this->pop_stack();
      Immediate val2 = this->pop_stack();
      Immediate val1 = this->pop_stack();

      if (c.v.n32 != 0) {
        this->push_stack(val1);
      } else {
        this->push_stack(val2);
      }

    } else if (instr.op == OpCode::Else) {
      // We have landed in a Else block, which we do not want to execute
      // We know that we can skip this block, because if the if block would have
      // taken the else route, skip_block would have stoped at the first op to
      // actually execute after the else
      skip_control_block(block, pc);
      continue;
    } else if (instr.op == OpCode::Call) {
      execute_function(instr.imms[0].v.n32);
    } else if (instr.op == OpCode::CallIndirect) {

      const Immediate &x = instr.imms[0]; // What table to use
      /* TODO: actually use x */
      /* TODO: why is signature index required? For call frame? */

      Immediate table_index = this->pop_stack(); // index in table

      assert(table_index.v.n32 < this->function_table.size() &&
             "invalid function table index!");

      uint32_t ref_function_index = this->function_table[table_index.v.n32];

      execute_function(ref_function_index);
    } else if (instr.op == OpCode::I32Const || instr.op == OpCode::F32Const ||
               instr.op == OpCode::I64Const || instr.op == OpCode::F64Const) {
      this->push_stack(instr.imms[0]);
    }
    /* Memory Instructions */
    else if (instr.op == OpCode::MemorySize) {
      Immediate pages;
      pages.t = ImmediateRepr::I32;
      pages.v.n32 = this->pages;
      this->push_stack(pages);
    } else if (instr.op == OpCode::MemoryGrow) {
      Immediate grow_by = this->pop_stack();

      // for some reason old page size is returned...
      Immediate old_pages;
      old_pages.t = ImmediateRepr::I32;
      old_pages.v.n32 = this->pages;
      this->push_stack(old_pages);

      pages += grow_by.v.n32;
      // TODO: check for failure, like not enough memory.
      memory.resize(pages * MEMORY_PAGE_SIZE);
    } else if (instr.op == MemoryFill) {
      Immediate n = this->pop_stack();
      Immediate val = this->pop_stack();
      Immediate i = this->pop_stack();

      // TODO: validate memory overflow

      OpCode i32store8 = OpCode::I32Store8;
      Immediate memidx = instr.imms[0];

      for (int j = 0; j < n.v.n32; j++) {
        uint32_t offset = i.v.n32 + j;
        handle_store(i32store8, memidx.v.n32, offset, val);
      }
    } else if (instr.op == MemoryCopy) {

      Immediate n = this->pop_stack();
      Immediate i2 = this->pop_stack();
      Immediate i1 = this->pop_stack();

      OpCode i32Load8u = OpCode::I32Load8U;
      OpCode i32store8 = OpCode::I32Store8;

      for (int j = 0; j < n.v.n32; j++) {
        if (i1.v.n32 <= i2.v.n32) {
          uint32_t load_offset = i2.v.n32 + j;
          Immediate imm =
              handle_load(i32Load8u, instr.imms[1].v.n32, load_offset);

          uint32_t store_offset = i1.v.n32 + j;
          handle_store(i32store8, instr.imms[0].v.n32, store_offset, imm);
        } else {
          for (int j = n.v.n32 - 1; j >= 0; j--) {
            uint32_t load_offset = i2.v.n32 + j;
            Immediate imm =
                handle_load(i32Load8u, instr.imms[1].v.n32, load_offset);

            uint32_t store_offset = i1.v.n32 + j;
            handle_store(i32store8, instr.imms[0].v.n32, store_offset, imm);
          }
        }
      }

    }
    else if(instr.op == MemoryInit) {
      Immediate n = this->pop_stack();
      Immediate j = this->pop_stack();
      Immediate i = this->pop_stack();

      uint32_t data_segment_index = instr.imms[0].v.n32;
      assert(data_segment_index < data.size() && "invalid data segment index");

      for(int h = 0; h < n.v.n32; h++) {
          uint32_t data_segment_byte_index = h+j.v.n32;

          assert(data_segment_byte_index < data[data_segment_index].bytes.size() && "invalid data segment byte index");

          Immediate byte;
          byte.t = ImmediateRepr::Byte;
          byte.v.n32 = static_cast<uint32_t>(data[data_segment_index].bytes[data_segment_byte_index]);

          uint32_t store_offset = i.v.n32 + h;
          OpCode i32store8 = OpCode::I32Store8;
          handle_store(i32store8, instr.imms[1].v.n32, store_offset, byte);
      }
    } else if(instr.op == DataDrop) {
      uint32_t data_segment_index = instr.imms[0].v.n32;
      assert(data_segment_index < data.size() && "invalid data segment index");
      DataSegment& seg = data[data_segment_index];
      // Drop bytes, TODO: future requests should trap 
      seg.bytes.clear();
    }
    /* STORE Instructions */
    else if (op_byte >= 0x36 && op_byte <= 0x3E) {

      if (instr.imms.size() > 2) {
        assert(false &&
               "todo: there are 3 immediates to I32Store, special case.");
      }

      Immediate x = instr.imms[0];
      Immediate ao = instr.imms[1];

      Immediate c = this->pop_stack();
      Immediate i = this->pop_stack();

      uint32_t offset = ao.v.n32 + i.v.n32;
      handle_store(instr.op, x.v.n32, offset, c);
    }
    /* Load operations */
    else if (op_byte >= 0x28 && op_byte <= 0x35) {

      if (instr.imms.size() > 2) {
        assert(
            false &&
            "todo: there are 3 immediates to a load operation, special case.");
      }

      Immediate x = instr.imms[0];
      Immediate ao = instr.imms[1];

      Immediate i = this->pop_stack();

      uint32_t offset = ao.v.n32 + i.v.n32;
      this->push_stack(handle_load(instr.op, x.v.n32, offset));
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
    } else if (instr.op == OpCode::LocalTee) {
      Immediate val = this->pop_stack();

      // Push twice, but executing LocalSet after will pop the last one again
      this->push_stack(val);

      uint32_t index = instr.imms[0].v.n32;
      if (index < params.size()) {
        params[index] = val;
      } else {
        index = index - params.size();
        assert(index < locals.size() && " LocalSet invalid local index!");
        locals[index] = val;
      }
    } else if (instr.op == OpCode::GlobalGet) {
      uint32_t index = instr.imms[0].v.n32;
      assert(index < globals.size() && "invalid globals access");
      this->push_stack(globals[index].value);
    } else if (instr.op == OpCode::GlobalSet) {
      Immediate val = this->pop_stack();
      uint32_t index = instr.imms[0].v.n32;

      assert(index < globals.size() && "invalid globals access");
      assert(globals[index].mut && "invalid set to globals");
      assert(globals[index].value.t == val.t && "invalid type set to globals");

      globals[index].value = val;
    }
    /* Convertion, Promotion, Demotion */
    else if (op_byte >= 0xA7 && op_byte <= 0xBB) {
      Immediate a = this->pop_stack();
      this->push_stack(handle_conversion(instr.op, a));
    }
    /* UNOP NUMERIC INSTRUCTIONS */
    else if (is_i32_numeric_unop || is_f32_numeric_unop ||
             is_f64_numeric_unop || is_i64_numeric_unop) {
      Immediate a = this->pop_stack();
      Immediate b;
      if (is_f32_numeric_unop) {
        b = handle_numeric_unop_f32(instr.op, a);
      } else if (is_f64_numeric_unop) {
        b = handle_numeric_unop_f64(instr.op, a);
      } else if (is_i32_numeric_unop) {
        b = handle_numeric_unop_i32(instr.op, a);
      } else if (is_i64_numeric_unop) {
        b = handle_numeric_unop_i64(instr.op, a);
      } else {
        assert(false && "todo!");
      }
      this->push_stack(b);
    }
    /* BINOP NUMERIC INSTRUCTIONS */
    else if (is_i32_numeric_binop || is_f32_numeric_binop ||
             is_f64_numeric_binop || is_i64_numeric_binop) {
      Immediate b = this->pop_stack();
      Immediate a = this->pop_stack();

      Immediate c;

      if (is_i32_numeric_binop) {
        c = handle_numeric_binop_i32(instr.op, a, b);
      } else if (is_i64_numeric_binop) {
        c = handle_numeric_binop_i64(instr.op, a, b);
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
        // execute second block
        // find Else statement or end statement, ignore else/end statements of
        // nested ifs!
        skip_control_block(block, pc);
        // dont include pc++ below, this would skip the next meaningfull op
        continue;
      }
    } else if (instr.op == OpCode::Loop) {
      const Immediate &bt = instr.imms[0];
      assert(bt.v.n32 == 0x40 &&
             "todo: only void as bt for loops currently supported.");

    } else if (instr.op == OpCode::Block) {
      assert((instr.imms[0].v.n32 == 0x7f || instr.imms[0].v.n32 == 0x40) &&
             "todo: currently only i32 blocks or 'e' blocks are allowed.");
      /* TODO: what is "val"? */
      /* This needs to be popped from the stack */
    } else if (instr.op == OpCode::Br) {
      // Exit block!
      const Immediate &label = instr.imms[0];
      assert(label.t == ImmediateRepr::I32 && "todo: wrong type assumed.");

      branch_block(block, pc, label.v.n32);
      continue; // we do not want to include the last pc++; pc is already at the
                // next instruction
    } else if (instr.op == OpCode::BrIf) {

      Immediate c = this->pop_stack();

      if (c.v.n32 != 0) {
        const Immediate &label = instr.imms[0];
        assert(label.t == ImmediateRepr::I32 && "todo: wrong type assumed.");

        branch_block(block, pc, label.v.n32);
        continue; // we do not want to include the last pc++; pc is already at
                  // the next instruction
      } else {
        // Do nothing
      }
    } else if (instr.op == OpCode::BrTable) {

      Immediate i = this->pop_stack();

      assert(i.v.n32 < instr.imms.size());

      if (i.v.n32 < instr.imms.size() - 1) {
        branch_block(block, pc, instr.imms[i.v.n32].v.n32);
      } else {
        // Use default, aka last item in immediates
        branch_block(block, pc, instr.imms.back().v.n32);
      }
      continue;

    } else if (instr.op == OpCode::Return) {
      break;
    } else {
      assert(false && "todo: implement new opcode emulation");
    }

    pc++;
  }
}

void Runtime::execute_block(const std::vector<Instr> &block) {
  std::vector<Immediate> params;
  std::vector<Immediate> locals;
  execute_block(block, params, locals);
}


void Runtime::execute_import(int function_index) {

  /* TODO: do fancy lookeup, but currently only support fd_write */

  Immediate nwritten = pop_stack();
  Immediate iovs_len = pop_stack();
  Immediate iovs_ptr = pop_stack();
  Immediate fd = pop_stack();

  uint32_t written = 0;

  for(int i = 0; i < iovs_len.v.n32; i++) {
    Immediate base_addr = read_memory(0, iovs_ptr.v.n32 + i*8, ImmediateRepr::I32);
    Immediate len = read_memory(0, iovs_ptr.v.n32 + i*8 + 4, ImmediateRepr::I32);

    std::string chunk(&this->memory[base_addr.v.n32], &this->memory[base_addr.v.n32 + len.v.n32]);
    if(fd.v.n32 == 1) {
      std::cout << chunk << std::endl;
    } else {
      std::cerr << chunk << std::endl;
    }
    written += len.v.n32;
  }

  Immediate written_result;
  written_result.t = ImmediateRepr::I32;
  written_result.v .n32= written;
  write_memory(0, nwritten.v.n32,  written_result);

  Immediate success;
  success.t = ImmediateRepr::I32;
  success.v.n32 = 0;
  push_stack(success);
}

void Runtime::execute_function(int function_index) {

  if(function_index < wasm.imports.size()) {
    return execute_import(function_index);
  } else {
    function_index -= wasm.imports.size();
  }

  // Again, assumes all indices are valid...
  const Code &block = wasm.codes[function_index];

  typeidx function_signature_index = wasm.function_section[function_index];

  // important for stack information
  const FunctionType &signature = wasm.type_section[function_signature_index];

  /* Pop Stack based on signature params */
  std::vector<Immediate> params(signature.params.size());
  // Go in reverse, first popped is actually last param!
  // how did i get this far without noticing problems in the first 65 tests...
  for (int i = signature.params.size() - 1; i >= 0; i--) {
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

  execute_block(block.expr, params, locals);
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
  this->execute_function(function_index);
}
