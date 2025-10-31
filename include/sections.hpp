#ifndef SECTIONS_HPP
#define SECTIONS_HPP

#include <cstdint>
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

typedef uint32_t typeidx;

// https://webassembly.github.io/spec/core/appendix/index-types.html

enum class Types : uint8_t {
  // Number types
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
  V128 = 0x7B,

  // Heap Type
  Noexn = 0x74,
  Nofunc = 0x73,
  Noextern = 0x72,
  None = 0x71,
};

bool is_valid_heap_type(uint8_t type);

struct FunctionType {
  std::vector<Types> params;
  Types return_value;
};

// Stores the data of the varios sections
struct wasm {
  std::vector<FunctionType> type_section;
  std::vector<typeidx> function_section;
};

// returns the name corresponding to the section id
const char *section_name(uint8_t section);

// Parses the Type Section and stores the resulting types in wasm.type_section
void parse_type_section(wasm &wasm, const std::vector<uint8_t> &data);

// Stores the resulting function indices in wasm.function_section
void parse_functions(wasm &wasm, const std::vector<uint8_t> &data);

#endif // SECTIONS_HPP
