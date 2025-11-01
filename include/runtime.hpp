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
    struct WasmFile& wasm;

    std::vector<Immediate> stack;
    std::vector<uint8_t> memory;

    // Pushes the stack by imm
    void push_stack(Immediate& imm);

    // Returns and removes the last value on the stack
    Immediate pop_stack();

    // Writes the number to the memory in little-endian bytes https://webassembly.github.io/spec/core/exec/numerics.html#storage
    // mem_index denotes the index of the memory to use of the store
    void write_memory(const uint32_t& mem_index, const uint32_t& offset, const Immediate& imm);


    // Executes the function given by its index, storing the results on the stack or memory.
    // It takes an index because function information such as parameters and actual body are stored in different structs in wasm
    void execute(int function_index);

  public:
    Runtime(struct WasmFile& wasm);

    // Takes as input the name of a function, looks it up in the exports and executes it.
    void run(std::string& function);


    // Reads from memory at offset, currently mem_index is ignored due to missing store impl.
    Immediate read_memory(const uint32_t& mem_index, const uint32_t& offset, const ImmediateRepr repr);
};



#endif // RUNNER_HPP
