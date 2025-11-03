# WebAssembly 1.0 Interpreter

This is a non-complete implementation of a WebAssembly 1.0 interpreter.
Currently it supports functions, local and global variables, memory, 
function table with element segments for initialization, exports and a very first step towards imports. 

It currently does not support
  - Any imports which are not `fd_write`
  - traps (the interpreter contains debug assertions but no graceful error handling)
  - Nontrapping Float-to-Int Conversion
  - Most OpCodes which are not present in the test files
  - An actual Store, e.g. mem indices are ignored and all memory is mapped to a single memory array.

## Running the tests
  The tests available from the assignment have been converted to GoogleTest assertions.
  They are implemented in the `tests/` directory, with the corresponding binaries in `test_binaries/`.

  I developed the code on Ubuntu but verified on Windows using the MSVC compiler.
  - Build the project using `cmake`
    - `mkdir build && cd build`
    - `cmake -G "Visual Studio 17 2022" -A x64 ..`
    - `cmake --build .` or `cmake --build . --config Release`
  - Run the tests `."/Debug/winterp_tests.exe"` or `."/Release/winterp_tests.exe"`
    - the executable must be run in the build folder, since the '.wasm' binaries will be copied there
  
  The interpreter should now successfully pass the implemented tests of
  
  - `01_test.wat`
  - `02_test_prio1.wat`
  - `03_test_prio2.wat` (except of trap tests, these are not implemented)
  - `04_test_prio3.wat`
  - `05_test_complex.wat`
  -  not implemented
  - `07_test_bulk_memory.wat`
  -  not implemented
  - `09_print_hello.wat`

## Parsing a WASM file 
  ### Sections
  `include/sections.hpp` contains the `class WasmFile`, which is responsible for parsing `.wasm` files.
  It stores all the important information like function signatures, exports, globals and the WebAssembly itself.
  A parser for a section usually looks like this
  ```c++
    void WasmFile::parse_type_section(const std::vector<uint8_t> &data) {
      const uint8_t *ptr = &data[0];
      const uint8_t *end = data.data() + data.size();
      const int num_types = uleb128_decode<uint32_t>(ptr, end);

      this->type_section.resize(num_types);

      for (int i = 0; i < num_types; i++) {
        // Parse each individual type and store in this->type_section
      }
    }
  ```
  An important function for parsing has been `uleb128_decode<uint32_t>(ptr, end)`, which works for both `uint32_t` and `uint64_t`.
  Of course there are also versions for signed ints, bytes and floats. All parsing and decoding functions take a pointer to the data and move the pointer to the next byte, not belonging to the parsed data.

  For understanding the structure of the Wasm file, using wat2wasm with -v was quite helpful in combination with the offical documentation.

  ### Expressions
  An expression is a list of instructions.
  For this project, I decided to represent Instructions with a struct.  
  ```c++
  enum OpCode;
  struct Immediate;

  struct Instr {
    OpCode op;
    std::vector<Immediate> imms;
  };
  ```
  In my opinion this is a more data friendly way, another approach would have been to have 
  a base class `Instruction` which would have been inherited by lots of child classes like `Nop`, `I32Add`.   
  I chose the struct version because having a child class for so many instructions would have quickly become infeasable to manage.
  Furthermore, it should, even though not really relevant in this assignment, have a beneficial performance impact, since everything is stored consecutively in memory.
  Whereas for a class based approach only the pointers to the objects would have been consecutively in memory.

  In `src/instructions.cpp`, I have a function which defined how many immediates are required to be parsed for each `OpCode`.
  This made it easy to have a single function `Instr parse_instruction(const uint8_t *&start, const uint8_t *end)` responsible for reading the correct amount of bytes for each instruction and its immediates.

## Runtime
  
  The runtime starts with initialising the globals, locals, memory and function table.
  All according to the parsed `WasmFile`.

  Then we enter execution when `Runtime.run(std::string &function)` is called.
  
  This looks up the function name in the exports and finds the corresponding function_index.
  There is no Abstract Syntax Tree or Control Flow Graph, the runtime runs directly on the list of instructions. This is mostly due to time constraints, but stepping through the instructions ended up being fairly simple to implement.
  The most important functions in `include/runtime.hpp` are
  
  - `void Runtime::execute_block(...);`
    Responsible for stepping through the instructions step by step. Calls the functions below to control the program counter.
  - `void Runtime::skip_control_block(...);`
    Skips through a control block by moving the program counter until the matching `else` or `end` is found. For the implementation it was important to ignore, or correctly count if the next else or end is from a nested block.
  - `void Runtime::branch_block(...);`
    Similar to skip_control_block, but dependent on `block` or `loop` will go to the start or end of a label.
    For `block` it escapes the block while for a `loop` it will go to its first instruction inside the loop.
  - `void push_stack(...);`, `Immediate pop_stack();`
    Handle stack operations while checking for missing types and asserting that there is an element when popping.
  - `void Runtime::write_memory(...);`
    Writes to memory, currently ignores memory index, but this wouldnt be a big change to support.
  - `Immediate Runtime::read_memory(...);`
    Reads from memory, again, ignoring mem_index.

  What made the runtime quite a bit simpler was the data structure of Immediates.
  An immediate would be stored like such
  ```c++
    enum ImmediateRepr {
      Byte = 0x01,
      I32 = 0x7F,
      I64 = 0x7E,
      F32 = 0x7D,
      F64 = 0x7C,
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
  ```
  This made it quite easy to handle the stack and more explicit when loading and storing values from memory.
  Due to the stack push and pop functions always asserting Immediates having a valid type, I could always be sure that there are no unknown values floating around. 
  For most OpCodes, working with immediates now looks like this
  ```c++
    Immediate result;
    result.t = ImmediateRepr::I32;
    if (op == OpCode::I32Add) {
      result.v.n32 = a.v.n32 + b.v.n32;
    } else if (...) {
      
    }
  ```

## Challenges
  - Imports: Due to running out of time, my interpreter only supports a single import.
    For more imports, I would need to map module and field name to their counterpart WASI functions.
  - Parsing the file: In the beginning I believed parsing a file wouldnt take me as long as it ended up doing, so I was unable to get started on the runtime before saturday.  
  - Getting other projects to run: I tried to find WebAssembly projects which my interpreter would be able to run.
    I alread knew from the beginning that this would most likely be impossible, since I only support `fd_write` and most projects are meant to have a GUI.
    But I'm fairly confident that simpler things like this [rust example](https://wasmbyexample.dev/examples/hello-world/hello-world.rust.en-us.html) could be possible.
    For other things like [mandelbrot module](https://www.assemblyscript.org/examples/mandelbrot.html) it would be interesting to see if adding the missing imports for `Math.log` and `Math.log2` would be enough. 
    This should be fairly straightforward by adapting the `execute_import` function. 

### What I would do differently or improve
  - Better compiler flags? Switching to MSVC gave me better warnings for when untyped conversions happen. While using clang my code compiles without any warnings.
  - Further make use of the Immediate types. For operations, it could be beneficial to always assert for correct types.
  - Gracious error handling: The Code currently simply crashes in debug mode when invalid executions would happen. For release mode the behaviour could be undefined.
    There would need to be a refactor to introduce error code or exceptions. 
    I personally would would choose error codes and pass these around, this would mean that any current return value would need to be added as a parameter reference. 
  - Implement a correct Store structure. For the given test cases there was no need to do this, but for a correct runtime this would be required.
  - The interpreter is very much focused around the test files, instructions not covered in the tests will most likely crash the program.
  - For a production system I would also have liked to explore an LLVM approach.
