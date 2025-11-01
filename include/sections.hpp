#ifndef SECTIONS_HPP
#define SECTIONS_HPP

#include "instructions.hpp"
#include <cstdint>
#include <string>
#include <vector>

// The reason i dont use a enum here is simply because its used exactly once
// different to HeapType which is used at different locations
const uint8_t CUSTOM_SECTION = 0;
const uint8_t TYPE_SECTION = 1;
const uint8_t IMPORT_SECTION = 2;
const uint8_t FUNCTION_SECTION = 3;
const uint8_t TABLE_SECTION = 4;
const uint8_t MEMORY_SECTION = 5;
const uint8_t GLOBAL_SECTION = 6;
const uint8_t EXPORT_SECTION = 7;
const uint8_t START_SECTION = 8;
const uint8_t ELEMENT_SECTION = 9;
const uint8_t CODE_SECTION = 10;
const uint8_t DATA_SECTION = 11;
const uint8_t DATA_COUNT_SECTION = 12;
const uint8_t TAG_SECTION = 13;

// returns the name corresponding to the section id
const char *section_name(uint8_t section);

// TODO: remove
typedef uint32_t typeidx;

// https://webassembly.github.io/spec/core/appendix/index-types.html


bool is_valid_heap_type(uint8_t type);

struct FunctionType {
  std::vector<ImmediateRepr> params;
  ImmediateRepr return_value;
};

struct Memory {
  uint8_t flag;
  uint64_t n;       // Minimum / start
  uint64_t maximum; // Only used for limit flags 0x01 and 0x05
};

struct Global {
  ImmediateRepr valtype;
  bool mutability;
  std::vector<Instr> expr;
};

enum ExportKind : uint8_t {
  func = 0x00,
  table = 0x01,
  mem = 0x02,
  global = 0x03,
  tag = 0x04,
};

struct Export {
  std::string name;
  ExportKind kind;
  uint32_t idx;
};

struct Local {
  uint32_t count;
  ImmediateRepr type;
};

struct Code {
  std::vector<Local> locals;
  std::vector<Instr> expr;
};

// Stores the data of the varios sections
struct WasmFile {

  private:

    // Reads in a valtype as defined in
    // https://webassembly.github.io/spec/core/binary/types.html#value-types
    ImmediateRepr read_valtype(const uint8_t* &ptr, const uint8_t* end);

    // Parses the Type Section and stores the resulting types in wasm.type_section
    void parse_type_section(const std::vector<uint8_t> &data);

    // Stores the resulting function indices in wasm.function_section
    void parse_functions(const std::vector<uint8_t> &data);

    // Stores the list of memories into wasm.memory
    void parse_memory(const std::vector<uint8_t> &data);

    // Stores the list of globals into wasm.global
    void parse_global(const std::vector<uint8_t> &data);

    // Stores the list of exports into wasm.exports
    void parse_exports(const std::vector<uint8_t> &data);

    // Stores the list of codes into wasm.codes
    void parse_code(const std::vector<uint8_t> &data);

  public:
    std::vector<FunctionType> type_section;
    std::vector<typeidx> function_section;
    std::vector<Memory> memory;
    std::vector<Export> exports;
    std::vector<Code> codes;

    int read(const char* file);
};



#endif // SECTIONS_HPP
