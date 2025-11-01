#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#include <cstdint>
#include <vector>

enum OpCode {
  // Parametric
  Unreachable = 0x00,
  Nop = 0x01,
  Drop = 0x1A,
  Select = 0x1B,
  Select_t = 0x1C, // Followed by a list  

  // Numeric Instructions
  I32Const = 0x41,
  I64Const = 0x42,
  F32Const = 0x43,
  F64Const = 0x44,
  GT_S = 0x4A,

  F32Add = 0x92,
  F32Sub = 0x93,
  F32Mul = 0x94,

  Call = 0x10,
  ReturnCall = 0x12,
  ReturncalIndirect = 0x13,

  // Memory Instructions
  I32Load = 0x28,
  I32Store = 0x36,
  MemorySize = 0x3F,
  MemoryGrow = 0x40,

  // Variable Instructions
  LocalGet = 0x20,
  LocalSet = 0x21,

  // Control Instructions
  If = 0x04,
  Else = 0x05,
  Return = 0x0F,

  End = 0x0b,
};

enum ImmediateRepr : uint8_t {
  Empty = 0,
  I32 = 1,
  I64 = 2,
  F32 = 3,
  F64 = 4
};

union Immediate {
  uint32_t n32; 
  uint64_t n64; 
  float p32;
  double p64;
};

struct Instr {
  OpCode op;
  std::vector<Immediate> imms;
};

// Reads the opcode and depending on it reads multiple immediates to finally return the Instr
Instr parse_instruction(const uint8_t* &start, const uint8_t* end);

// Reads all instructions starting from ptr until 0x0b (end code) is read and builds the expression vector result.
void read_expr(const uint8_t* &ptr, const uint8_t* end, std::vector<Instr>& result);

#endif // INSTRUCTIONS_HPP
 
