#include "instructions.hpp"
#include "leb128.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

// Returns the expected immediates of a OpCode
// Assuming it has maximally 2, edge cases must be handles differently
void immediates(OpCode op, ImmediateRepr &imm0, ImmediateRepr &imm1,
                ImmediateRepr &imm2) {

  imm0 = ImmediateRepr::Empty;
  imm1 = ImmediateRepr::Empty;
  imm2 = ImmediateRepr::Empty;

  // Numeric instructions without any immediates
  if(static_cast<uint8_t>(op) >= 0x45 && static_cast<uint8_t>(op) <= 0xC4) {
    return;
  }

  switch (op) {
    // No immediates
  case OpCode::Unreachable:
  case OpCode::GT_S:
  case OpCode::Else:
  case OpCode::Return:
  case OpCode::Nop:
  case OpCode::Drop:
    break;

    // Single immediate I32
  case OpCode::I32Const:
  case OpCode::MemorySize:
  case OpCode::MemoryGrow:
  case OpCode::Call:
  case OpCode::ReturnCall:
  case OpCode::LocalSet:
  case OpCode::LocalGet:
  case OpCode::If:
    imm0 = ImmediateRepr::I32;
    break;

  // Single immediate I64
  case OpCode::I64Const:
    imm0 = ImmediateRepr::I64;
    break;

  // Two immediates I32, I32
  case OpCode::I32Store:
  case OpCode::ReturncalIndirect:
    imm0 = ImmediateRepr::I32;
    imm1 = ImmediateRepr::I32;
    break;

  // Single Immediate, F32
  case OpCode::F32Const:
    imm0 = ImmediateRepr::F32;
    break;

  // Single Immediate, F64
  case OpCode::F64Const:
    imm0 = ImmediateRepr::F64;
    break;

  // Missing implementation
  default: {
    std::cout << "OpCode Immediates missing!" << std::hex << op << std::endl;
    assert(false && "todo");
  }
  };
}

Immediate parse_immediate(const ImmediateRepr repr, const uint8_t *&start,
                          const uint8_t *end) {

  assert(repr != ImmediateRepr::Empty && "Invalid immediate repr parsed.");

  Immediate imm;
  if (repr == ImmediateRepr::I32) {
    imm.n32 = uleb128_decode<uint32_t>(start, end);
  } else if (repr == ImmediateRepr::I64) {
    imm.n64 = uleb128_decode<uint64_t>(start, end);
  } else if (repr == ImmediateRepr::F32) {
    imm.p32 = read_float(start, end);
  } else if (repr == ImmediateRepr::F64) {
    imm.p64 = read_double(start, end);
  }

  return imm;
}

// For memargs, depending on the value N, there can be two or three immediates
// https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions
void parse_memarg(const uint8_t *&start, const uint8_t *end, Instr& instr) {
  Immediate n = parse_immediate(ImmediateRepr::I32, start, end);
  instr.imms.push_back(n);

  if(n.n32 >= 64) { // 2^6
   // x
   instr.imms.push_back(parse_immediate(ImmediateRepr::I32, start, end));
  } 

  // m
   instr.imms.push_back(parse_immediate(ImmediateRepr::I64, start, end));
}

Instr parse_instruction(const uint8_t *&start, const uint8_t *end) {
  Instr instr;
  instr.op = static_cast<OpCode>(read_byte(start, end));

  if (instr.op == OpCode::End) {
    return instr;
  }

  // Instructions using memarg
  if(static_cast<uint8_t>(instr.op) >= 0x28 && static_cast<uint8_t>(instr.op) <= 0x3E) {
    parse_memarg(start, end, instr);
    return instr;
  }

  ImmediateRepr imm0, imm1, imm2;
  immediates(instr.op, imm0, imm1, imm2);

  // TODO: maybe precount to use resize
  // the reason i dont use fixed imm0-2 in Instr is due to edge cases requiring
  // lists
  if (imm0 != ImmediateRepr::Empty) {
    instr.imms.push_back(parse_immediate(imm0, start, end));
  }

  if (imm1 != ImmediateRepr::Empty) {
    instr.imms.push_back(parse_immediate(imm1, start, end));
  }

  if (imm2 != ImmediateRepr::Empty) {
    instr.imms.push_back(parse_immediate(imm2, start, end));
  }

  return instr;
}

void read_expr(const uint8_t *&ptr, const uint8_t *end,
               std::vector<Instr> &result) {
  Instr instr = parse_instruction(ptr, end);

  // for all condition instruction 0x02, 0x03, 0x04, this will be increased by
  // one, then when we exit the if/else, a End code will be read, for this End
  // Code, we do not want to stop reading in the remaining instructions
  // therefore we only exit the loop when we know we are not currently in a
  // nested condition
  int conditional_nesting = 0;
  while (instr.op != OpCode::End || conditional_nesting > 0) {

    uint8_t op = static_cast<uint8_t>(instr.op);
    if (0x02 <= op && op <= 0x04) {
      // Not inclusive 0x05, because else does not continue the nesting
      conditional_nesting++;
    }

    if (instr.op == OpCode::End) {
      conditional_nesting--;
    } else {
      // End OpCodes do not add any execution value
      result.push_back(instr);
    }

    instr = parse_instruction(ptr, end);
  }
}
