#include "frat.h"

void main(uint32_t largs, char **arg){
	// shellinit();
	// while(true){
	// 	char *bf = readbuf(64, "\n>> ");
	// 	cmd_errout info = shell(bf);
	// 	printf("\n\t[%u]: %s", info.errcode, info.msg);
	// 	free(bf);
	// }
	// exit(EXIT_SUCCESS);


	conf_fsroot *fr;
	if(!(fr = fmount(arg[1]))){
		GPTeNSTR *str = makeGPTeNSTR("Root");
		formatpart(*str);
		if(!(fr = fmount(arg[1]))){
			printf("Drive Error! Killing Program");
			exit(EXIT_FAILURE);
		}
		printf("\n\n");
		free(str);
	}
	dirhandle *dir = __floaddir(fr, "ripping", "Fdc\0");

	fhandle *handle = fsloadh(fr, "ripping/attempt\0", "Ffc");
	__fdirrefresh(dir);
	printf("\n\nFtest Output: %s\n\n", (__ftest(handle) ? "TRUE": "FALSE"));
	meta_fsblock *finfo_ = _freadinfo(handle);		__fprint_info(finfo_);		free(finfo_);
	fsuloadh(handle);

	dirrunner *dr = __dirr_init(dir, "***");
	unhandle *uh;
	while((uh = __dirr(dr))){
		meta_fsblock *fb;
		if(uh->dir){
			fb = _dreadinfo(uh->dhandle_);			__fuloaddir(uh->dhandle_);
		}else{fb = _freadinfo(uh->fhandle_);		fsuloadh(uh->fhandle_);}
		__fprint_info(fb);
		free(fb);
	}
	__dirr_free(dr);
	__fuloaddir(dir);
}