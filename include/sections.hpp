#ifndef SECTIONS_H
#define SECTIONS_H

#include <cstdint>


// WebAssembly section IDs
const uint8_t CUSTOM_SECTION      = 0;
const uint8_t TYPE_SECTION        = 1;
const uint8_t IMPORT_SECTION      = 2;
const uint8_t FUNCTION_SECTION    = 3;
const uint8_t TABLE_SECTION       = 4;
const uint8_t MEMORY_SECTION      = 5;
const uint8_t GLOBAL_SECTION      = 6;
const uint8_t EXPORT_SECTION      = 7;
const uint8_t START_SECTION       = 8;
const uint8_t ELEMENT_SECTION     = 9;
const uint8_t CODE_SECTION        = 10;
const uint8_t DATA_SECTION        = 11;
const uint8_t DATA_COUNT_SECTION  = 12;
const uint8_t TAG_SECTION         = 13;

// returns the name corresponding to the section id
const char* section_name(uint8_t section); 

#endif // SECTIONS_H
