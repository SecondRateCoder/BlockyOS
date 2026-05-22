#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)

#define EXECSECRTIONREFPATHLEN 16

typedef uint32_t __execsecref_attributes_t;

enum {
    __noliveload  = 0x1,
    __noload      = 0x2,
    __required    = 0x4,
    __noreloc     = 0x8,
    __reloctable  = 0x10,
};

typedef struct {
    char   path[EXECSECRTIONREFPATHLEN];
    __execsecref_attributes_t attributes;
    uint32_t parameter;
    uint64_t bOffset;   // file offset of section data
    uint64_t nBytes;    // size of section data
    uint64_t confBASE;  // preferred base (optional)
} execsectionref;

typedef struct {
    char magic[16];        // exechmagic
    char versionmagic[16]; // exechvermagic
    uint64_t attributes;
    uint64_t imageBase;
    uint16_t nSections;
    // followed by execsectionref[nSections]
} exech;

// Simple relocation entry in our format
typedef struct {
    uint64_t byteLoc;   // offset within section
    uint8_t  ptrSize;   // 8 for DIR64
    uint8_t  type;      // original PE type (e.g. IMAGE_REL_BASED_DIR64)
    uint16_t reserved;
} my_reloc_entry;

#pragma pack(pop)
