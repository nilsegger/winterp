#include "instructions.hpp"
#include "leb128.hpp"
#include "sections.hpp"
#include <cassert>
#include <iostream>

bool is_valid_heap_type(uint8_t type) {
  return
      // Number Types
      (type >= 0x7B && type <= 0x7F) | (type >= 0x71 && type <= 0x74);
}

const char *section_name(uint8_t section) {
  switch (section) {
  case 0:
    return "custom";
  case 1:
    return "type";
  case 2:
    return "import";
  case 3:
    return "function";
  case 4:
    return "table";
  case 5:
    return "memory";
  case 6:
    return "global";
  case 7:
    return "export";
  case 8:
    return "start";
  case 9:
    return "element";
  case 10:
    return "code";
  case 11:
    return "data";
  case 12:
    return "data count";
  case 13:
    return "tag";
  default:
    return "unknown";
  }
}

void parse_type_section(wasm &wasm, const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_types = uleb128_decode<uint32_t>(ptr, end);

  wasm.type_section.resize(num_types);

  for (int i = 0; i < num_types; i++) {
    const uint8_t type = uleb128_decode<uint32_t>(ptr, end);
    assert(type == 0x60 && "wasm file not matching specifications, only "
                           "functions in types is supported.");

    FunctionType f;

    // HeapTypes are encoded as typeidx for this section, therefore uint32_t is
    // correct
    // https://webassembly.github.io/spec/core/binary/modules.html#type-section
    const uint8_t num_params = uleb128_decode<uint32_t>(ptr, end);
    f.params.resize(num_params);

    for (int j = 0; j < num_params; j++) {
      uint32_t type = uleb128_decode<uint32_t>(ptr, end);
      assert(is_valid_heap_type(type) && "Invalid heap type found for param!");
      f.params[j] = static_cast<Types>(type);
    }

    const uint8_t num_results = uleb128_decode<uint32_t>(ptr, end);
    assert(num_results <= 1 &&
           "wasm does not support more than one return value.");

    if (num_results == 0) {
      f.return_value = Types::None;
    } else {
      uint32_t type = uleb128_decode<uint32_t>(ptr, end);
      assert(is_valid_heap_type(type) &&
             "Invalid heap type found for return value!");
      f.return_value = static_cast<Types>(type);
    }

    wasm.type_section[i] = f;
  }
}

void parse_functions(wasm &wasm, const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_functions = uleb128_decode<uint32_t>(ptr, end);

  wasm.function_section.resize(num_functions);

  for (int i = 0; i < num_functions; i++) {
    wasm.function_section[i] = uleb128_decode<uint32_t>(ptr, end);
  }
}

void parse_memory(wasm &wasm, const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_memories = uleb128_decode<uint32_t>(ptr, end);
  wasm.memory.resize(num_memories);

  for (int i = 0; i < num_memories; i++) {
    Memory m;
    m.flag = uleb128_decode<uint32_t>(ptr, end);
    m.n = uleb128_decode<uint64_t>(ptr, end);    
    if(m.flag == 0x01 || m.flag == 0x05) {
      m.maximum = uleb128_decode<uint64_t>(ptr, end);    
    } else {
      m.maximum = 0;
    }

    wasm.memory[i] = m;
  }
}


// Stores the list of globals into wasm.global
void parse_global(wasm &wasm, const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_globals = uleb128_decode<uint32_t>(ptr, end);
  wasm.memory.resize(num_globals);

  for(int i = 0; i < num_globals; i++) {

    uint32_t valtype = uleb128_decode<uint32_t>(ptr, end);

    if(valtype != 0x7C && valtype != 0x7D && valtype != 0x7E && valtype != 0x7F) {
      // TODO: implement https://webassembly.github.io/spec/core/binary/types.html#value-types
      assert(false && "valtype not yet supported!!");
    }

    Global g;
    g.valtype = static_cast<Types>(valtype);
    g.mutability = uleb128_decode<uint8_t>(ptr, end);

    Instr instr = parse_instruction(ptr, end);
    while(instr.op != OpCode::End) {
      g.expr.push_back(instr);     
      instr = parse_instruction(ptr, end);
    }
   }
}
