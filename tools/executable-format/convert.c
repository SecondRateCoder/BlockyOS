#include "convert.h"

typedef struct{uint8_t *data;		size_t size;}filebuf;

static int read_file(const char *path, filebuf *out){
	FILE *f = fopen(path, "rb");
	if (!f){return -1;}
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	if(sz < 0){fclose(f);		return -1;}
	fseek(f, 0, SEEK_SET);
	uint8_t *buf = (uint8_t *)malloc((size_t)sz);
	if(!buf){fclose(f);      return -1;}

	if(fread(buf, 1, (size_t)sz, f) != (size_t)sz){
		free(buf);
		fclose(f);
		return -1;
	}
	fclose(f);
	out->data = buf;
	out->size = (size_t)sz;
	return 0;
}

static uint32_t rva_to_file_offset(uint32_t rva, IMAGE_SECTION_HEADER *secs, uint16_t nsecs){
	for(uint16_t i = 0; i < nsecs; ++i){
		uint32_t va   = secs[i].VirtualAddress;
		uint32_t size = secs[i].Misc.VirtualSize ?
						secs[i].Misc.VirtualSize : secs[i].SizeOfRawData;
		if(rva >= va && rva < va + size){
			uint32_t delta = rva - va;
			return secs[i].PointerToRawData + delta;
		}
	}
	return 0;
}

static int convert_pe_to_exec(const char *inpath, const char *outpath){
	filebuf fb = {0};
	if(read_file(inpath, &fb) != 0){
		fprintf(stderr, "Failed to read %s\n", inpath);
		return -1;
	}
	uint8_t *base = fb.data;
	size_t   size = fb.size;
	if(size < sizeof(IMAGE_DOS_HEADER)){
		fprintf(stderr, "File too small\n");
		free(base);
		return -1;
	}
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
	if(dos->e_magic != 0x5A4D){ // "MZ"
		fprintf(stderr, "Not MZ\n");
		free(base);
		return -1;
	}
	if((size_t)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > size){
		fprintf(stderr, "Invalid e_lfanew\n");
		free(base);
		return -1;
	}
	IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
	if(nt->Signature != 0x00004550){ // "PE\0\0"
		fprintf(stderr, "Not PE\n");
		free(base);
		return -1;
	}
	if(nt->OptionalHeader.Magic != 0x20B){ // PE32+
		fprintf(stderr, "Not PE32+\n");
		free(base);
		return -1;
	}
	uint16_t nsecs = nt->FileHeader.NumberOfSections;
	IMAGE_SECTION_HEADER *secs = (IMAGE_SECTION_HEADER *)((uint8_t *)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
	// Locate base relocation directory
	IMAGE_DATA_DIRECTORY *reloc_dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	uint32_t reloc_rva  = reloc_dir->VirtualAddress;
	uint32_t reloc_size = reloc_dir->Size;
	uint32_t reloc_off = rva_to_file_offset(reloc_rva, secs, nsecs);
	if (!reloc_off || reloc_size == 0) {
		fprintf(stderr, "No base relocations\n");
		// We still can emit sections, but no relocations
	}
	// Prepare output file
	FILE *out = fopen(outpath, "wb");
	if (!out) {
		fprintf(stderr, "Failed to open %s\n", outpath);
		free(base);
		return -1;
	}
	// Emit:
	// [exech][execsectionref * N][section data...][reloc sections...]
	exech hdr;
	memset(&hdr, 0, sizeof(hdr));
	memcpy(hdr.magic,        "MYEXECFMT\0\0\0\0\0\0\0", 16);
	memcpy(hdr.versionmagic, "VER1\0\0\0\0\0\0\0\0\0\0\0", 16);
	hdr.attributes = 0;
	hdr.imageBase  = nt->OptionalHeader.ImageBase;
	// For each PE section we create one data section + one reloc section
	uint16_t n_data_secs  = nsecs;
	uint16_t n_reloc_secs = nsecs; // one per section, may be empty
	hdr.nSections = n_data_secs + n_reloc_secs;
	// Reserve space for header + section table
	long header_pos = ftell(out);
	fwrite(&hdr, 1, sizeof(hdr), out);
	execsectionref *sectab = (execsectionref *)calloc(hdr.nSections, sizeof(execsectionref));
	if (!sectab) {
		fclose(out);
		free(base);
		return -1;
	}
	// We'll fill sectab, then rewrite it after we know offsets
	long sectab_pos = ftell(out);
	fwrite(sectab, sizeof(execsectionref), hdr.nSections, out);
	// Emit data sections
	uint16_t sec_index = 0;
	for(uint16_t i = 0; i < nsecs; ++i){
		execsectionref *s = &sectab[sec_index];
		memset(s, 0, sizeof(*s));
		// Name
		char name[9] = {0};
		memcpy(name, secs[i].Name, 8);
		snprintf(s->path, EXECSECRTIONREFPATHLEN, "%s", name);
		s->attributes = 0;
		s->parameter  = 0;
		s->confBASE   = nt->OptionalHeader.ImageBase + secs[i].VirtualAddress;
		s->bOffset = (uint64_t)ftell(out);
		s->nBytes  = secs[i].SizeOfRawData;
		if (s->nBytes && secs[i].PointerToRawData &&
			secs[i].PointerToRawData + s->nBytes <= size) {
			fwrite(base + secs[i].PointerToRawData, 1, s->nBytes, out);
		}
		sec_index++;
	}
	// Emit relocation sections (one per original section)
	// We will scan the base relocation table and bucket entries by section.
	my_reloc_entry **rel_buckets = (my_reloc_entry **)calloc(nsecs, sizeof(my_reloc_entry *));
	size_t *rel_counts = (size_t *)calloc(nsecs, sizeof(size_t));
	size_t *rel_caps = (size_t *)calloc(nsecs, sizeof(size_t));
	if(reloc_off && reloc_size){
		uint32_t off = reloc_off;
		uint32_t end = reloc_off + reloc_size;
		while (off + sizeof(IMAGE_BASE_RELOCATION) <= end) {
			IMAGE_BASE_RELOCATION *blk = (IMAGE_BASE_RELOCATION *)(base + off);
			if (blk->SizeOfBlock == 0) break;
			uint32_t page_rva = blk->VirtualAddress;
			uint32_t block_sz = blk->SizeOfBlock;
			uint32_t entries_off = off + sizeof(IMAGE_BASE_RELOCATION);
			uint32_t entries_end = off + block_sz;
			while(entries_off + 2 <= entries_end){
				uint16_t entry = *(uint16_t *)(base + entries_off);
				entries_off += 2;
				uint16_t type   = (entry >> 12) & 0xF;
				uint16_t offset = entry & 0x0FFF;
				if(type == IMAGE_REL_BASED_ABSOLUTE){continue;}// skip

				if(type == IMAGE_REL_BASED_DIR64){
					uint32_t rva = page_rva + offset;
					// Find which section this RVA belongs to
					for(uint16_t si = 0; si < nsecs; ++si){
						uint32_t va   = secs[si].VirtualAddress;
						uint32_t size_sec =
							secs[si].Misc.VirtualSize ?
							secs[si].Misc.VirtualSize :
							secs[si].SizeOfRawData;
						if(rva >= va && rva < va + size_sec){
							uint32_t sec_off = rva - va;
							// Append to bucket si
							if (rel_counts[si] == rel_caps[si]) {
								size_t newcap = rel_caps[si] ? rel_caps[si] * 2 : 16;
								my_reloc_entry *nb = (my_reloc_entry *)realloc(rel_buckets[si], newcap * sizeof(my_reloc_entry));
								if(!nb){
									fclose(out);
									free(base);
									free(sectab);
									for (uint16_t k = 0; k < nsecs; ++k){free(rel_buckets[k]);}
									free(rel_buckets);
									free(rel_counts);
									free(rel_caps);
									return -1;
								}
								rel_buckets[si] = nb;
								rel_caps[si]    = newcap;
							}
							my_reloc_entry *re = &rel_buckets[si][rel_counts[si]++];
							re->byteLoc  = sec_off;
							re->ptrSize  = 8; // DIR64
							re->type     = (uint8_t)type;
							re->reserved = 0;
							break;
						}
					}
				}else{}// Other relocation types can be added here if needed
			}
			off += blk->SizeOfBlock;
		}
	}
	// Now write relocation sections
	for(uint16_t i = 0; i < nsecs; ++i){
		execsectionref *s = &sectab[sec_index];
		memset(s, 0, sizeof(*s));
		char name[9] = {0};
		memcpy(name, secs[i].Name, 8);
		snprintf(s->path, EXECSECRTIONREFPATHLEN, "rel.%s", name);
		s->attributes = __reloctable;
		s->parameter  = 0;
		s->confBASE   = 0;
		s->bOffset = (uint64_t)ftell(out);
		s->nBytes  = (uint64_t)(rel_counts[i] * sizeof(my_reloc_entry));

		if(s->nBytes && rel_buckets[i]){
			fwrite(rel_buckets[i], sizeof(my_reloc_entry), rel_counts[i], out);
		}

		sec_index++;
	}
	// Rewrite header + section table
	fseek(out, header_pos, SEEK_SET);
	fwrite(&hdr, 1, sizeof(hdr), out);
	fseek(out, sectab_pos, SEEK_SET);
	fwrite(sectab, sizeof(execsectionref), hdr.nSections, out);
	fclose(out);
	for(uint16_t i = 0; i < nsecs; ++i){free(rel_buckets[i]);}
	free(rel_buckets);
	free(rel_counts);
	free(rel_caps);
	free(sectab);
	free(base);
	return 0;
}