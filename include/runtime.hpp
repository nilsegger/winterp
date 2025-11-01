#ifndef RUNNER_HPP
#define RUNNER_HPP

#include "instructions.hpp"
#include "sections.hpp"
#include <vector>

// As defined per
// https://webassembly.github.io/spec/core/exec/runtime.html#memory-instances
const int MEMORY_PAGE_SIZE = 65536;

class Runtime {

  private:
    // the parsed data file
    const struct WasmFile& wasm;

    std::vector<Immediate> stack;
    std::vector<uint8_t> memory;

    // Returns and removes the last value on the stack
    Immediate pop_stack();

    // Writes the number to the memory in little-endian bytes https://webassembly.github.io/spec/core/exec/numerics.html#storage
    // mem_index denotes the index of the memory to use of the store
    void write_memory(const uint32_t& mem_index, const uint32_t& offset, const Immediate& imm);

    
    // Moves the program counter, until the next else / end is found, ignoring
    // nested conditions
    // The program counter will be left at the first instruction to be executed next.
    void skip_block(const Code &c, int &pc);

    // Computes the resulting Immediate based on the value of OpCode
    // The valid OpCodes for this function are limited to binop's for i32
    Immediate handle_numeric_binop_i32(const OpCode& op, const Immediate& a, const Immediate& b);

    // Computes the resulting Immediate based on the value of OpCode
    // The valid OpCodes for this function are limited to unop's for f32
    Immediate handle_numeric_unop_f32(const OpCode& op, const Immediate& a);

    // Computes the resulting Immediate based on the value of OpCode
    // The valid OpCodes for this function are limited to binop's for f32
    // Resulting immediate is not limited to f32, result of comparisons will be set to i32
    Immediate handle_numeric_binop_f32(const OpCode& op, const Immediate& a, const Immediate& b);

    // Executes the function given by its index, storing the results on the stack or memory.
    // It takes an index because function information such as parameters and actual body are stored in different structs in wasm
    void execute(int function_index);

  public:
    Runtime(const struct WasmFile& wasm);

    // Takes as input the name of a function, looks it up in the exports and executes it.
    void run(std::string& function);


    // Reads from memory at offset, currently mem_index is ignored due to missing store impl.
    Immediate read_memory(const uint32_t& mem_index, const uint32_t& offset, const ImmediateRepr repr);

    
    // Pushes the stack by imm
    void push_stack(const Immediate& imm);

};



#endif // RUNNER_HPP
