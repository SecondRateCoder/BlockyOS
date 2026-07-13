#include "exec.h"

static void *resolve_custom_image_base(socket_t *root, socket_t *file, char **args, size_t nArgs, resolveArgs *callArgs){
	(void)root;
	(void)args;
	(void)nArgs;
	(void)callArgs;

	if(!file || !file->persistent){return NULL;}
	unhandle *handle = (unhandle *)file->persistent;
	if(!handle || !handle->fhandle_){return NULL;}

	UINTN file_size = __fsize(handle->fhandle_);
	if(file_size < sizeof(BExecHeader)){return NULL;}

	void *file_data = __calloc(1, file_size);
	if(!file_data){return NULL;}

	UINTN read_bytes = _fread(handle->fhandle_, file_size, &file_data);
	if(read_bytes != file_size){
		__free(file_data);
		return NULL;
	}

	BExecHeader *header = (BExecHeader *)file_data;
	if(memcmp(header->JsonManifest, "BEXEC", 5) != 0){
		__free(file_data);
		return NULL;
	}

	const size_t section_table_size = sizeof(BExecHeader) + (header->nSections * sizeof(BExecFileItem));
	size_t payload_offset = (section_table_size + 7u) & ~((size_t)7u);
	BExecFileItem *sections = header->FileItems;
	void *resolved_image = __calloc(1, file_size);
	if(!resolved_image){
		__free(file_data);
		return NULL;
	}

	for(UINT64 i = 0; i < header->nSections; ++i){
		if(sections[i].name[0] == '\0'){continue;}
		if(sections[i].flags & BExecFlags__Resource){
			// Mount Resource Section
			continue;
		}

		if(__memcmp(sections[i].name, DLLPREFIX, strlen(DLLPREFIX)) == 0){
			// Remember the DLL section and resolve the imported symbols from it later.
			continue;
		}

		if(sections[i].flags & BExecFlags__RelocReference){
			const uint8_t *reloc_bytes = (const uint8_t *)file_data + payload_offset;
			const uint32_t *reloc_words = (const uint32_t *)reloc_bytes;
			for(size_t j = 0; j < sections[i].nBytes / sizeof(uint32_t); ++j){
				uint32_t target_index = reloc_words[j];
				if(target_index < header->nSections){
					if(sections[target_index].flags & BExecFlags__Resource){
						// Mount Resource Section
					}else if(__memcmp(sections[target_index].name, DLLPREFIX, strlen(DLLPREFIX)) == 0){
						// Resolve the target symbol from the remembered DLL section.
					}else if((sections[target_index].flags & BExecFlags__Executable) || (sections[target_index].flags & BExecFlags__Data)){
						memcpy((uint8_t *)resolved_image + payload_offset, (const uint8_t *)file_data + payload_offset, sections[target_index].nBytes);
					}
				}
			}
			continue;
		}

		if((sections[i].flags & BExecFlags__Executable) || (sections[i].flags & BExecFlags__Data)){
			memcpy((uint8_t *)resolved_image + payload_offset, (const uint8_t *)file_data + payload_offset, sections[i].nBytes);
		}

		payload_offset = (payload_offset + sections[i].nBytes + 7u) & ~((size_t)7u);
	}

	__free(file_data);
	return resolved_image;
}

void *resolve(socket_t *root, socket_t *file, char **args, size_t nArgs, resolveArgs *callArgs){
	return resolve_custom_image_base(root, file, args, nArgs, callArgs);
}

