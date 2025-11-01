#include "instructions.hpp"
#include "leb128.hpp"
#include "sections.hpp"
#include <cassert>
#include <iostream>
#include <fstream>

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

ImmediateRepr WasmFile::read_valtype(const uint8_t* &ptr, const uint8_t* end) {
    uint32_t valtype = uleb128_decode<uint32_t>(ptr, end);

    if (valtype != 0x7C && valtype != 0x7D && valtype != 0x7E &&
        valtype != 0x7F) {
      // TODO: implement
      // https://webassembly.github.io/spec/core/binary/types.html#value-types
      std::cout << "Unsupported valtype " << std::hex << valtype << std::endl;
      assert(false && "valtype not yet supported!!");
    }

  return static_cast<ImmediateRepr>(valtype);
}

void WasmFile::parse_type_section(const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_types = uleb128_decode<uint32_t>(ptr, end);

  this->type_section.resize(num_types);

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
      f.params[j] = static_cast<ImmediateRepr>(type);
    }

    const uint8_t num_results = uleb128_decode<uint32_t>(ptr, end);
    assert(num_results <= 1 &&
           "wasm does not support more than one return value.");

    if (num_results == 0) {
      f.return_value = ImmediateRepr::None;
    } else {
      uint32_t type = uleb128_decode<uint32_t>(ptr, end);
      assert(is_valid_heap_type(type) &&
             "Invalid heap type found for return value!");
      f.return_value = static_cast<ImmediateRepr>(type);
    }

    this->type_section[i] = f;
  }
}

void WasmFile::parse_functions(const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_functions = uleb128_decode<uint32_t>(ptr, end);

  this->function_section.resize(num_functions);

  for (int i = 0; i < num_functions; i++) {
    this->function_section[i] = uleb128_decode<uint32_t>(ptr, end);
  }
}

void WasmFile::parse_memory(const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_memories = uleb128_decode<uint32_t>(ptr, end);
  this->memory.resize(num_memories);

  for (int i = 0; i < num_memories; i++) {
    Memory m;
    m.flag = uleb128_decode<uint32_t>(ptr, end);
    m.n = uleb128_decode<uint64_t>(ptr, end);
    if (m.flag == 0x01 || m.flag == 0x05) {
      m.maximum = uleb128_decode<uint64_t>(ptr, end);
    } else {
      m.maximum = 0;
    }

    this->memory[i] = m;
  }
}


void WasmFile::parse_global(const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_globals = uleb128_decode<uint32_t>(ptr, end);
  this->memory.resize(num_globals);

  for (int i = 0; i < num_globals; i++) {

    Global g;
    g.valtype = read_valtype(ptr, end);
    g.mutability = uleb128_decode<uint8_t>(ptr, end);

    read_expr(ptr, end, g.expr);
  }
}

void WasmFile::parse_exports(const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_exports = uleb128_decode<uint32_t>(ptr, end);
  this->exports.resize(num_exports);

  for (int i = 0; i < num_exports; i++) {
    const uint32_t string_length = uleb128_decode<uint32_t>(ptr, end);

    Export e;
    e.name = std::string(ptr, ptr + string_length);
    ptr += string_length;

    uint8_t kind = uleb128_decode<uint8_t>(ptr, end);
    e.kind = static_cast<ExportKind>(kind);
    e.idx = uleb128_decode<uint32_t>(ptr, end);

    this->exports[i] = e;
  }
}

void WasmFile::parse_code(const std::vector<uint8_t> &data) {
  const uint8_t *ptr = &data[0];
  const uint8_t *end = data.data() + data.size();
  const int num_functions = uleb128_decode<uint32_t>(ptr, end);
  this->codes.resize(num_functions);

  for(int i = 0; i < num_functions; i++) {
    uint32_t func_body_size = uleb128_decode<uint32_t>(ptr, end);
    uint32_t locals = uleb128_decode<uint32_t>(ptr, end);

    Code c;
    c.locals.resize(locals);

    for(int j = 0; j < locals; j++) {
      c.locals[j].count = uleb128_decode<uint32_t>(ptr, end);
      c.locals[j].type = read_valtype(ptr, end);
    }

    read_expr(ptr, end, c.expr);

    this->codes[i] = c;
  }

}

int WasmFile::read(const char* file_name) {
  
  std::ifstream file(file_name, std::ios::binary);

  if (!file) {
    std::cerr << "unable to open " << file_name << std::endl;
    return 1;
  }

  std::vector<uint8_t> header(8); // Read Magic and Version
  file.read((char *)header.data(), header.size());
  if (file.gcount() != 8) {
    std::cerr << file_name << "corruped file" << std::endl;
    return 1;
  }

  const unsigned char expected_magic[4] = {0x00, 0x61, 0x73, 0x6D};
  if (!std::equal(header.begin(), header.begin() + 4, expected_magic)) {
    std::cerr << "is not a valid WebAssembly binary file" << std::endl;
    return 1;
  }

  // Section information based on
  // https://webassembly.github.io/spec/core/binary/modules.html

  uint8_t section_id;
  uint32_t section_size;

  do {
    file.read((char *)&section_id, sizeof(section_id));
    section_size = file_uleb128_u32t(file);

    std::cout << "; section " << section_name(section_id) << std::endl;
    std::cout << static_cast<int>(section_size) << "\t\t\t; section_size"
              << std::endl;

    // assumes code files are not larger that working memory...
    std::vector<uint8_t> section_data(section_size);
    file.read((char *)section_data.data(), section_size);

    // TODO: lookup table? only more ergonomic...
    if (section_id == TYPE_SECTION) {
      parse_type_section(section_data);
    } else if (section_id == FUNCTION_SECTION) {
      parse_functions(section_data);
    } else if (section_id == MEMORY_SECTION) {
      parse_memory(section_data);
    } else if (section_id == GLOBAL_SECTION) {
      parse_global(section_data);
    } else if (section_id == EXPORT_SECTION) {
      parse_exports(section_data);
    } else if (section_id == CODE_SECTION) {
      parse_code(section_data);
    }

  } while (file.peek() != EOF);

  return 0;
}
