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

#endif // INSTRUCTIONS_HPP
 
