#include "convert.h"
#include "exec.h"
#include "pe.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "import/stb_image.h"
#include "import/stb_image_resize2.h"
#include "import/stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
	if(argc < 3){
		fprintf(stderr, "usage: %s <input> [<output>] [-d] [-json <PATH TO JSON MANIFEST>]\n", argv[0]);
		return EXIT_FAILURE;
	}
	const char *in = NULL, *out = NULL, *jsonmanifest = NULL;
	for(uint32_t cc = 1; cc < argc; ++cc){
		if(argv[cc][0] == '-'){
			switch (argv[cc][1]){
				case 'd': {
					FILE *f;
					if(out){
						if(f = fopen(out, "r")){/*dump_exec(out);*/	fclose(f);}
					}else if(in){/*dump_exec(out);*/}
				} case 'j': {
					if(strcmp(argv[cc], "-json")){
						if(stat(argv[cc + 1], NULL)){
							jsonmanifest = calloc(ManifestJsonLen, sizeof(char));
							memcpy(jsonmanifest, argv[cc + 1], strlen(argv[cc + 1]));
							continue;
						}
					}
				}
				default: {break;}
			}
		}else{
			FILE  *f;
			if(!in){
				if(f = fopen(argv[cc], "r")){
					in = argv[cc];
					fclose(f);
				}
			}else{if(in && !out){out = argv[cc];}}
		}
	}
	
	ExpandedPeExecutable *epe = ExpandPeExecutableFormat(in);
	GenerateBeHeader(out, DefaultSystemSectionName, (ExecIcon){0}, "{}", 0);
	InitImportSection(out, epe, ".ImportS", 0);
	return EXIT_SUCCESS;
}