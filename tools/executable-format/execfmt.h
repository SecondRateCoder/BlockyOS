#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* =========================
 *  PE / COFF definitions
 * ========================= */

#pragma pack(push, 1)

typedef struct IMAGE_DOS_HEADER{
    uint16_t e_magic;    // "MZ" = 0x5A4D
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    int32_t  e_lfanew;   // file offset to PE header
}IMAGE_DOS_HEADER;

typedef struct IMAGE_DATA_DIRECTORY{
    uint32_t   VirtualAddress;
    uint32_t   Size;
}IMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct IMAGE_FILE_HEADER{
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
}IMAGE_FILE_HEADER;

typedef struct IMAGE_OPTIONAL_HEADER64_LIKE{
    uint16_t Magic; // 0x10B = PE32, 0x20B = PE32+
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    // PE32 only:
    uint32_t BaseOfData;
    // NT additional fields (PE32/PE32+ differ here)
    uint64_t ImageBase; // for PE32 this is 32-bit, but we store in 64
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
}IMAGE_OPTIONAL_HEADER64_LIKE; // we’ll adapt for 32/64

typedef struct IMAGE_NT_HEADERS_ANY{
    uint32_t Signature; // "PE\0\0" = 0x00004550
    IMAGE_FILE_HEADER FileHeader;
    // Optional header follows (PE32 or PE32+)
}IMAGE_NT_HEADERS_ANY;

typedef struct IMAGE_SECTION_HEADER{
    uint8_t  Name[8];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
}IMAGE_SECTION_HEADER;

typedef struct IMAGE_BASE_RELOCATION{
    uint32_t VirtualAddress;
    uint32_t SizeOfBlock;
}IMAGE_BASE_RELOCATION;

#pragma pack(pop)

#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5
#define IMAGE_REL_BASED_ABSOLUTE       0
#define IMAGE_REL_BASED_HIGHLOW        3
#define IMAGE_REL_BASED_DIR64          10

/* =========================
 *  ELF definitions
 * ========================= */

#pragma pack(push, 1)

#define EI_NIDENT 16

typedef struct Elf64_Ehdr{
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint64_t      e_entry;
    uint64_t      e_phoff;
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
}Elf64_Ehdr;

typedef struct Elf64_Phdr{
    uint32_t   p_type;
    uint32_t   p_flags;
    uint64_t   p_offset;
    uint64_t   p_vaddr;
    uint64_t   p_paddr;
    uint64_t   p_filesz;
    uint64_t   p_memsz;
    uint64_t   p_align;
}Elf64_Phdr;

typedef struct Elf32_Ehdr{
    unsigned char e_ident32[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;
    uint32_t      e_phoff;
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
}Elf32_Ehdr;

typedef struct Elf32_Phdr{
    uint32_t   p_type;
    uint32_t   p_offset;
    uint32_t   p_vaddr;
    uint32_t   p_paddr;
    uint32_t   p_filesz;
    uint32_t   p_memsz;
    uint32_t   p_flags;
    uint32_t   p_align;
}Elf32_Phdr;

#pragma pack(pop)

/* ELF constants */
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define EI_CLASS 4
#define ELFCLASS32 1
#define ELFCLASS64 2

#define PT_LOAD 1