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
  LT_S = 0x48,

  I32Add = 0x6a,
  I32Sub = 0x6b,
  I32Mul = 0x6C,

  F32Add = 0x92,
  F32Sub = 0x93,
  F32Mul = 0x94,
  F32Div = 0x95,
  F32Min = 0x96,
  F32Max = 0x97,
  F32Abs = 0x8B,
  F32Neg = 0x8C,
  F32Ceil = 0x8D,
  F32Floor = 0x8E,
  F32Trunc = 0x8F,
  F32Nearest = 0x90,
  F32Sqrt = 0x91,

  F32EQ = 0x5B,
  F32Ne = 0x5C,
  F32Lt = 0x5D,
  F32Gt = 0x5E,
  F32Le = 0x5F,
  F32Ge = 0x60,

  I32ReinterpF32 = 0xBC,

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

  Uninitialised = 0x00,
  
  // Number types
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,

  // Heap Type
  Noexn = 0x74,
  Nofunc = 0x73,
  Noextern = 0x72,
  None = 0x71,

  // reftype
  // https://webassembly.github.io/spec/core/binary/types.html#reference-types
  RefHt = 0x64,
  RefNullHt = 0x63
};

union Value {
  uint32_t n32; 
  uint64_t n64; 
  float p32;
  double p64;
};

struct Immediate {
  ImmediateRepr t;
  Value v;
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
 
