#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#include <cstdint>
#include <vector>

enum OpCode {
  // Parametric
  Unreachable = 0x00,
  Nop = 0x01,
  Block = 0x02,
  Loop = 0x03,
  Br = 0x0C,
  BrIf = 0x0D,
  BrTable = 0x0E,

  Drop = 0x1A,
  Select = 0x1B,
  Select_t = 0x1C, // Followed by a list

  // Numeric Instructions
  I32Const = 0x41,
  I64Const = 0x42,
  F32Const = 0x43,
  F64Const = 0x44,

  I32eqz = 0x45, // unop!
  I32eq = 0x46,
  I32ne = 0x47,
  I32lts = 0x48,
  I32ltu = 0x49,
  I32gts = 0x4A,
  I32gtu = 0x4B,
  I32le_s = 0x4C,
  I32le_u = 0x4D,
  I32ge_s = 0x4E,
  I32ge_u = 0x4F,

  
  I32clz = 0x67,
  I32ctz = 0x68,
  I32popcnt = 0x69,

  I32Add = 0x6a,
  I32Sub = 0x6b,
  I32Mul = 0x6C,
  I32DivS = 0x6D,
  I32DivU = 0x6E,
  I32RemS = 0x6F,
  I32RemU = 0x70,
  I32and = 0x71,
  I32or = 0x72,
  I32xor = 0x73,
  I32shl = 0x74,
  I32shrs = 0x75,
  I32shru = 0x76,
  I32rotl = 0x77,
  I32rotr = 0x78,
  

  I64Add = 0x7C,
  I64Sub = 0x7D,
  I64Mul = 0x7E,
  I64DivS = 0x7F,
  I64DivU = 0x80,
  I64RemS = 0x81,
  I64RemU = 0x82,
  I64and = 0x83,
  I64or = 0x84,
  I64xor = 0x85,
  I64shl = 0x86,
  I64shrs = 0x87,
  I64shru = 0x88,
  I64rotl = 0x89,
  I64rotr = 0x8A,

  I64clz = 0x79,
  I64ctz = 0x7A,
  I64popcnt = 0x7B,

  I64eqz = 0x50,
  I64eq = 0x51,
  I64ne = 0x52,
  I64lts = 0x53,
  I64ltu = 0x54,
  I64gts = 0x55,

  I64gtu = 0x56,
  I64les = 0x57,
  I64leu = 0x58,
  I64ges = 0x59,
  I64geu = 0x5A,

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

  F32CopySign = 0x98,

  F32EQ = 0x5B,
  F32Ne = 0x5C,
  F32Lt = 0x5D,
  F32Gt = 0x5E,
  F32Le = 0x5F,
  F32Ge = 0x60,

  F64Abs = 0x99,
  F64Neg = 0x9A,
  F64Ceil = 0x9B,
  F64Floor = 0x9C,
  F64Trunc = 0x9D,
  F64Nearest = 0x9E,
  F64Sqrt = 0x9F,
  F64Add = 0xA0,
  F64Sub = 0xA1,
  F64Mul = 0xA2,
  F64Div = 0xA3,
  F64Min = 0xA4,
  F64Max = 0xA5,
  F64CopySign = 0xA6,

  F64EQ = 0x61,
  F64Ne = 0x62,
  F64Lt = 0x63,
  F64Gt = 0x64,
  F64Le = 0x65,
  F64Ge = 0x66,

  I32WrapI64 = 0xA7,
  I32TruncSF32 = 0xA8,
  I32TruncUF32 = 0xA9,
  I32TruncSF64 = 0xAA,
  I32TruncUF64 = 0xAB,
  I64ExtendSI32 = 0xAC,
  I64ExtendUI32 = 0xAD,

  I64TruncSF32 = 0xAE,
  I64TruncUF32 = 0xAF,

  I64TruncSF64 = 0xB0,
  I64TruncUF64 = 0xB1,

  F32ConvertSI32 = 0xB2,
  F32ConvertUI32 = 0xB3,
  F32ConvertSI64 = 0xB4,
  F32ConvertUI64 = 0xB5,
  F32DemoteF64 = 0xB6,
  F64ConvertSI32 = 0xB7,
  F64ConvertUI32 = 0xB8,
  F64ConvertSI64 = 0xB9,
  F64ConvertUI64 = 0xBA,
  F32PromoteF64 = 0xBB,
  I32ReinterpF32 = 0xBC,
  I64ReinterpF64 = 0xBD,
  F32ReinterpI32 = 0xBE,
  F64ReinterpI64 = 0xBF,

  Call = 0x10,
  CallIndirect = 0x11,
  ReturnCall = 0x12,
  ReturncalIndirect = 0x13,

  // Memory Instructions
  I32Load = 0x28,
  I64Load = 0x29,
  F32Load = 0x2A,
  F64Load = 0x2B,
  I32Load8S = 0x2C,
  I32Load8U = 0x2D,
  I32Load16S = 0x2E,
  I32Load16U = 0x2F,
  I64Load8S = 0x30,
  I64Load8U = 0x31,
  I64Load16S = 0x32,
  I64Load16U = 0x33,
  I64Load32S = 0x34,
  I64Load32U = 0x35,

  I32Store = 0x36,
  I64Store = 0x37,
  F32Store = 0x38,
  F64Store = 0x39,
  I32Store8 = 0x3A,
  I32Store16 = 0x3B,
  I64Store8 = 0x3C,
  I64Store16 = 0x3D,
  I64Store32 = 0x3E,

  MemorySize = 0x3F,
  MemoryGrow = 0x40,

  // Variable Instructions
  LocalGet = 0x20,
  LocalSet = 0x21,
  LocalTee = 0x22,

  GlobalGet = 0x23,
  GlobalSet = 0x24,

  // Control Instructions
  If = 0x04,
  Else = 0x05,
  Return = 0x0F,

  End = 0x0b,
};

enum ImmediateRepr : uint8_t {

  Uninitialised = 0x00,

  Byte = 0x01,

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

// Reads the opcode and depending on it reads multiple immediates to finally
// return the Instr
Instr parse_instruction(const uint8_t *&start, const uint8_t *end);

// Reads all instructions starting from ptr until 0x0b (end code) is read and
// builds the expression vector result.
void read_expr(const uint8_t *&ptr, const uint8_t *end,
               std::vector<Instr> &result);

#endif // INSTRUCTIONS_HPP
