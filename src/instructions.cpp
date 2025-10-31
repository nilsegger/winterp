#include "instructions.hpp"
#include "leb128.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

// Returns the expected immediates of a OpCode
// Assuming it has maximally 2, edge cases must be handles differently
void immediates(OpCode op, ImmediateRepr& imm0, ImmediateRepr& imm1, ImmediateRepr& imm2) {

  imm0 = ImmediateRepr::Empty;
  imm1 = ImmediateRepr::Empty;
  imm2 = ImmediateRepr::Empty;
  
   switch(op) {
       case OpCode::I32Const:
        imm0 = ImmediateRepr::I32;
        break;
       case OpCode::I64Const:
        imm0 = ImmediateRepr::I64;
         break;
       default: {
         std::cout << "OpCode Immediates missing!" << std::hex << op << std::endl;  
         assert(false && "todo");
       }
   };
}

Immediate parse_immediate(const ImmediateRepr repr, const uint8_t *&start, const uint8_t *end) {
  Immediate imm;
  if(repr == ImmediateRepr::I32) {
    imm.n32 = static_cast<OpCode>(uleb128_decode<uint32_t>(start, end));
  } else if(repr == ImmediateRepr::I64) {
    imm.n64 = static_cast<OpCode>(uleb128_decode<uint64_t>(start, end));
  } else {

    std::cout << "Immediate " << std::dec << repr << std::endl;
    assert(false && "todo parse immediates");
  }  
  return imm;
}

Instr parse_instruction(const uint8_t *&start, const uint8_t *end) {
  Instr instr;
  instr.op = static_cast<OpCode>(uleb128_decode<uint8_t>(start, end));

  if (instr.op == OpCode::End) {
    return instr; 
  } 

  ImmediateRepr imm0, imm1, imm2;
  immediates(instr.op, imm0, imm1, imm2);

  // TODO: maybe precount to use resize
  // the reason i dont use fixed imm0-2 in Instr is due to edge cases requiring lists
  if(imm0 != ImmediateRepr::Empty) {
    instr.imms.push_back(parse_immediate(imm0, start, end));  
  }

  if(imm1 != ImmediateRepr::Empty) {
    instr.imms.push_back(parse_immediate(imm1, start, end));  
  }

  if(imm2 != ImmediateRepr::Empty) {
    instr.imms.push_back(parse_immediate(imm2, start, end));  
  }

  return instr;
}
