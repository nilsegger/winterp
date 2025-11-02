#ifndef RUNNER_HPP
#define RUNNER_HPP

#include "instructions.hpp"
#include "sections.hpp"
#include <cstdint>
#include <vector>

// As defined per
// https://webassembly.github.io/spec/core/exec/runtime.html#memory-instances
const int MEMORY_PAGE_SIZE = 65536;

class Runtime {

private:
  // the parsed data file
  const struct WasmFile &wasm;

  // Normal stack, can be pushed and popped.
  std::vector<Immediate> stack;

  // Array memory
  std::vector<uint8_t> memory;

  // The Data Segments, directly copied from WasmFile
  std::vector<DataSegment> data;

  struct GlobalInstance {
    bool mut;
    Immediate value;
  };

  std::vector<GlobalInstance> globals;

  // How many pages of memory we currently have. One page is MEMORY_PAGE_SIZE bytes
  uint32_t pages;

  // Initialised by the "Table" section in wasm
  std::vector<uint32_t> function_table;
  
  // Returns and removes the last value on the stack
  Immediate pop_stack();

  // Pushes the stack by imm
  void push_stack(const Immediate &imm);

  // Writes the number to the memory in little-endian bytes
  // https://webassembly.github.io/spec/core/exec/numerics.html#storage
  // mem_index denotes the index of the memory to use of the store
  void write_memory(const uint32_t &mem_index, const uint32_t &offset,
                    const Immediate &imm);

  // Moves the program counter, until the next else / end is found, ignoring
  // nested conditions
  // The program counter will be left at the first instruction to be executed
  // next.
  void skip_control_block(const std::vector<Instr> &block, int &pc);

  // Moves the program counter to the next executable instruction.
  // This means it searches for arity + 1 'end's, ignoring newly nested ends.
  void branch_block(const std::vector<Instr> &block, int &pc, int arity); 
  
  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to unop's for i32
  Immediate handle_numeric_unop_i32(const OpCode &op, const Immediate &a);

  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to binop's for i32
  Immediate handle_numeric_binop_i32(const OpCode &op, const Immediate &a,
                                     const Immediate &b);


  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to unop's for i64
  Immediate handle_numeric_unop_i64(const OpCode &op, const Immediate &a);

  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to binop's for i64
  Immediate handle_numeric_binop_i64(const OpCode &op, const Immediate &a,
                                     const Immediate &b);

  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to unop's for f32
  Immediate handle_numeric_unop_f32(const OpCode &op, const Immediate &a);

  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to binop's for f32
  // Resulting immediate is not limited to f32, result of comparisons will be
  // set to i32
  Immediate handle_numeric_binop_f32(const OpCode &op, const Immediate &a,
                                     const Immediate &b);

  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to unop's for f64
  Immediate handle_numeric_unop_f64(const OpCode &op, const Immediate &a);

  // Computes the resulting Immediate based on the value of OpCode
  // The valid OpCodes for this function are limited to binop's for f64
  // Resulting immediate is not limited to f64, result of comparisons will be
  // set to i32
  Immediate handle_numeric_binop_f64(const OpCode &op, const Immediate &a,
                                     const Immediate &b);

  // Converts, Promotes or demotes 'a' based on OpCode.
  Immediate handle_conversion(const OpCode &op, const Immediate &a);

  // Handles all Load operations with given reinterp
  Immediate handle_load(const OpCode &op, const uint32_t& mem_index, const uint32_t& offset);

  // Handles all store operations 
  void handle_store(const OpCode &op, const uint32_t& mem_index, const uint32_t& offset, const Immediate& value);

  // Changes the type of A from 'from' to 'to'. No casting or actual conversion
  // is done. Will assert that the current type of a is 'from'.
  Immediate reinterp(const Immediate &a, const ImmediateRepr from,
                     const ImmediateRepr to);

  // Executes the given instruction block
  // params and locals need to be correctly initialised, since these can be used by the block
  void execute_block(const std::vector<Instr>& block, std::vector<Immediate>& params, std::vector<Immediate>& locals);

  // Execute a block and initialised params and locals to be empty
  void execute_block(const std::vector<Instr>& block);
  
  // Executes the requested import function, these are provided by the "host", aka this interpreter
  void execute_import(int function_index); 

  // Executes the function given by its index, storing the results on the stack
  // or memory. It takes an index because function information such as
  // parameters and actual body are stored in different structs in wasm
  void execute_function(int function_index);

public:
  Runtime(const struct WasmFile &wasm);

  // Takes as input the name of a function, looks it up in the exports and
  // executes it.
  void run(std::string &function);

  // Reads from memory at offset, currently mem_index is ignored due to missing
  // store impl.
  Immediate read_memory(const uint32_t &mem_index, const uint32_t &offset,
                        const ImmediateRepr repr);

};

#endif // RUNNER_HPP
