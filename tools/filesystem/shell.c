#include "shell.h"

symtype_e togglesymtype(symtype_e value, bool set){
	volatile static symtype_e write;
	if(set){write = value;}
	return write;
}

cmddesc *getcmd(char *cmd){
	if(!cmd){return NULL;}
	for(size_t cc = 0; cc < ncommands; ++cc){
		if(!memcmp(commands[cc].alias, cmd, __min(strlen(cmd), 8)) || 
			!memcmp(commands[cc].cmd, cmd, __min(strlen(cmd), 8))
		){return (commands + cc);}
	}
	return NULL;
}

void *resolvedata(char *src){
	cmddesc *cd = getcmd("e.sym");
	symrefbuffer *buffer = cd->persistent;
	if(buffer){
		for(uint32_t cc = 0; cc < buffer->len; ++cc){
			if(strstr(src, buffer->symrefbuffer[cc].alias)){
				if(buffer->symrefbuffer[cc].type == symtype_e__string){
					return str_replace(src, buffer->symrefbuffer[cc].alias, buffer->symrefbuffer[cc].data);
				}else{return buffer->symrefbuffer[cc].data;}
			}
		}
	}
	return NULL;
}

cmd_errout __shellparse(char *input){
    strtok_t *tokstate = strtok_i(input, " \"", strtok__ForceSameBorderingDelims);
    char *tok = strtok_k(tokstate);
	cmddesc *cd = getcmd(tok);
    if(cd && cd->func){
        size_t *buffer = calloc(cd->flags.nflags, sizeof(size_t));
		while(tok = strtok_k(tokstate)){
			for(size_t cc = 0; cc < cd->flags.nflags; ++cc){
				if(!strcmp(tok, cd->flags.flags[cc].flag)){
					switch(cd->flags.flags[cc].type){
						case cmddescargtype__switch: {
							buffer[cc] = true;    
							break;
						} case cmddescargtype__bool: {
							char *temp = strtok_k(tokstate), *_temp = resolvedata(temp);
							char *arg = _temp ? _temp : temp;
							if(arg){
								if(!memcmp(arg, "true", 4) || !memcmp(arg, "TRUE", 4)){
									buffer[cc] = true;
								}else{buffer[cc] = false;}
							}
							break;
						} case cmddescargtype__char: {
							char *temp = strtok_k(tokstate), *_temp = resolvedata(temp);
							if(temp){buffer[cc] = *((char *)(_temp ? _temp : temp));}
							break;
						} 
						case cmddescargtype__byte:
						case cmddescargtype__word:
						case cmddescargtype__dword: {
							char *temp = strtok_k(tokstate), *_temp = resolvedata(temp);
							char *arg = _temp ? _temp : temp;
							if(arg){buffer[cc] = strtoul(arg, NULL, 10);}
							break;
						} case cmddescargtype__qword: {
							char *temp = strtok_k(tokstate), *_temp = resolvedata(temp);
							char *arg = _temp ? _temp : temp;
							if(arg){buffer[cc] = strtoull(arg, NULL, 10);}
							break;
						} case cmddescargtype__string: {
							char *temp = strtok_k(tokstate), *_temp = resolvedata(temp);
							buffer[cc] = (size_t)(_temp ? _temp : temp);
							break;
						} case cmddescargtype__longinteger: {
							char *temp = strtok_ff(tokstate), *_temp = resolvedata(temp);
							buffer[cc] = (size_t)(_temp ? _temp : temp);
							break;
						}
					}
				}
			}
		}
        cmd_errout eo = cd->func(buffer, &cd->persistent);
        strtok_d(tokstate);
        free(buffer);
        return eo;
    }
    return (cmd_errout){.errcode = 0, .msg = ""};
}

char *flagtypetostr(cmddesc_argtype type){
	switch(type){
		case cmddescargtype__switch: return "switch";
		case cmddescargtype__bool: return "boolean";
		case cmddescargtype__char: return "char";
		case cmddescargtype__byte: return "byte";
		case cmddescargtype__word: return "word";
		case cmddescargtype__dword: return "dword";
		case cmddescargtype__qword: return "qword";
		case cmddescargtype__string: return "string";
		case cmddescargtype__longinteger: return "long-data";
		default:    return "unknown";
	}
}

cmd_errout __shellhelp(size_t *buffer, void **persistent){
	char *command = ((char *)buffer[0]);
	if(!command){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *cd = getcmd(command);
	if(cd){
		dualprintf(logf, stdout, 
			"\n[%llu]"
			"\n\tCommand: %.8s"
			"\n\tAlias: %.8s"
			"\n\tFlags:",
			((size_t)cd - (size_t)((void *)commands)) / sizeof(cmddesc), cd->cmd, cd->alias
		);
		for(uint8_t cc_ = 0; cc_ < cd->flags.nflags; ++cc_){
			dualprintf(logf, stdout, 
				"\n\t\t{%.3s\t:\t%s\t:\t%s}",
				cd->flags.flags[cc_].flag,
				flagtypetostr(cd->flags.flags[cc_].type),
				cd->flags.flags[cc_].desc
			);
		}
		dualprintf(logf, stdout, "\n\tDesc: %s", cd->desc);
		return (cmd_errout){.errcode = 0, .msg = ""};
	}else if(!strncmp(command, "..", __min(strlen(command), 2))){
		for(size_t cc = 0; cc < ncommands; ++cc){
			dualprintf(logf, stdout, 
				"\n[%llu]"
				"\n\tCommand: %.8s"
				"\n\tAlias: %.8s"
				"\n\tFlags:",
				cc, commands[cc].cmd, commands[cc].alias
			);
			for(uint8_t cc_ = 0; cc_ < commands[cc].flags.nflags; ++cc_){
				dualprintf(logf, stdout, 
					"\n\t\t{%.3s\t:\t%s\t:\t%s}",
					commands[cc].flags.flags[cc_].flag,
					flagtypetostr(commands[cc].flags.flags[cc_].type),
					commands[cc].flags.flags[cc_].desc
				);
			}
			dualprintf(logf, stdout, "\n\tDesc: %s", commands[cc].desc);
		}
		return (cmd_errout){.errcode = 0, .msg = ""};
	}else if(!strncmp(command, "short", __min(strlen(command), 5))){
		for(size_t cc = 0; cc < ncommands; ++cc){
			dualprintf(logf, stdout, "\n [%llu]: %.8s(%.8s)\t", cc, commands[cc].cmd, commands[cc].alias);
			for(uint8_t cc_ = 0; cc_ < commands[cc].flags.nflags; ++cc_){
				dualprintf(logf, stdout, " %.3s(%s)", commands[cc].flags.flags[cc_].flag, flagtypetostr(commands[cc].flags.flags[cc_].type));
			}
		}
		return (cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Command does not exist"};
}

cmd_errout __shelldisk(size_t *buffer, void **persistent){
	// Initialise e.mount
	char *path = ((char *)buffer[0]), *partition = ((char *)buffer[1]);
	if(!path){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *cd = getcmd("e.mount");
	if(cd){
		cd->persistent = fmount(path);
		if(!cd->persistent){
			uint32_t confblocksize = buffer[2], conflogblocks = buffer[3];
			char *version = ((char *)buffer[4]);
			// Verify all Items
			if(!confblocksize){confblocksize = __FS_DEFAULTBLOCKSIZE;	dualprintf(logf, stdout, "\nEnabling default Block Size %u", __FS_DEFAULTBLOCKSIZE);}
			if(!conflogblocks){conflogblocks = __FS_DEFAULTLOGSECTORS;	dualprintf(logf, stdout, "\nEnabling default Log Sectors %u", __FS_DEFAULTLOGSECTORS);}
			if(!version){version = __FS_DEFAULTFORMATEDVERSION;			dualprintf(logf, stdout, "\nUsing Default Version: %s", __FS_DEFAULTFORMATEDVERSION);}
			if(partition){
				GPTeNSTR *str = makeGPTeNSTR(partition);
				// Get Version
				strtok_t *tstate = strtok_i(version, " :\"", 0);
				uint32_t vermajor = strtoul(strtok_k(tstate), NULL, 10), 
						verminor = strtoul(strtok_k(tstate), NULL, 10);
				strtok_d(tstate);
				formatpart(path, *str, confblocksize, conflogblocks, vermajor, verminor);
				free(str);
				cd->persistent = fmount(path);
				if(!cd->persistent){
					cmddesc *symcd = getcmd("e.sym");
					symcd->func((size_t *)((char *[2]){"REALDISK", strdup(path)}), &symcd->persistent);
					return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Failed to load Partition"};}
			}else{return (cmd_errout){.errcode  = cmddescerrtype_undefined_setting, .msg = "No Partition target was set."};}
		}
		cmddesc *symcd = getcmd("e.sym");
		symcd->func((size_t *)((char *[2]){"REALDISK", strdup(path)}), &symcd->persistent);
		return (cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Dependency \"e.mount\" is undefined"};
}

cmd_errout __shellcreate(size_t *buffer, void **persistent){
	// type:path:name
	char *type = (char *)(buffer[0]), *path = (char *)(buffer[1]), *name = (char *)(buffer[2]);
	dualprintf(logf, stdout, "\n%p:%s\t%p:%s\t%p:%s\t", type, type, path, path, name, name);
	if(!type || !path || !name){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	char *typedup = strdup(type);
	typedup = realloc(typedup, strlen(type) + 1);
	typedup[strlen(type)] = 'c';	typedup[strlen(type) + 1] = '\0';
	// Initialise Persistent
	// Use persistent in "e.mount"
	cmddesc *symcd = getcmd("e.sym"), *mountcd = getcmd("e.mount");
	if(symcd && mountcd){
		togglesymtype(symtype_e__fhandle, true);		char *npath = strdup(path);
		if(npath[strlen(path) - 1] == PATHSEP || npath[strlen(path) - 1] == PATHnoSEP){
			npath = realloc(npath, strlen(path) + strlen(name));
			memcpy(npath + strlen(path), name, strlen(name) + 1);
		}else{
			npath = realloc(npath, strlen(path) + strlen(name));
			npath[strlen(path)] = PATHSEP;
			memcpy(npath + strlen(path), name, strlen(name));
		}
		dualprintf(logf, stdout, "\n[\t%s]", npath);
		if(mountcd->persistent){symcd->func((size_t *)((char *[]){name, (char *)sizeof(fhandle), (char *)floadh(mountcd->persistent, npath, typedup)}), &symcd->persistent);}
	}else{free(typedup);	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "\nError... Could not set Persistent"};}
	free(typedup);
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellflush(size_t *buffer, void **persistent){
	for(size_t cc = 0; cc < ncommands; ++cc){
		free(commands[cc].persistent);
	}
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellsym(size_t *buffer, void **persistent){
	size_t nbytes = buffer[2];
	char *alias = (char *)(buffer[0]), *data = (char *)(buffer[1]);
	if(!nbytes || !alias || !data){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	symrefbuffer *srb = *persistent;
	if(!srb){srb = calloc(sizeof(symrefbuffer), 1);		*persistent = srb;		srb->len = 0;}
	srb->len++;
	if(srb->symrefbuffer){	// For some reason there's a Memory Corruption in reallocating srb->symrefbuffer, so for now we perform the realloc ourself
		void *temp = srb->symrefbuffer;
		srb->symrefbuffer = calloc(srb->len, sizeof(__symref));
		memcpy(srb->symrefbuffer, temp, (srb->len - 1) * sizeof(__symref));
		free(temp);
	}else{srb->symrefbuffer = calloc(sizeof(__symref), srb->len);}
	srb->symrefbuffer[srb->len - 1] = (__symref){
		.alias = calloc(strlen(alias) + 5, sizeof(char)),
		.data = data,
		.type = togglesymtype(0, false),
		.nbytes = (togglesymtype(0, false) == symtype_e__string? strlen(data): nbytes)
	};
	togglesymtype(symtype_e__string, true);
	snprintf(srb->symrefbuffer[srb->len - 1].alias, strlen(alias) + 5, "%%%s%%", alias);
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shelldelete(size_t *buffer, void **persistent){
	if(!(*buffer)){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	__ffremove(getcmd("e.mount")->persistent, resolvedata((char *)(*buffer)));
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellopen(size_t *buffer, void **persistent){
	// Attach Sym Handle
	char *loadargs = ((char *)buffer[0]), *path = ((char *)buffer[1]), *alias = ((char *)buffer[2]);
	if(!loadargs || !path || !alias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *symcd = getcmd("e.sym"), *mountcd = getcmd("e.mount");
	if(mountcd->persistent){
		// Disable string flag.
		togglesymtype(symtype_e__fhandle, true);
		fhandle *fh = floadh(mountcd->persistent, path, loadargs);
		symcd->func((size_t *)((char *[3]){alias, (char *)(fh? fh: NULL), (char *)sizeof(fhandle)}), &symcd->persistent);
		return(cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){
		.errcode = cmddescerrtype_undefined_setting,
		.msg = "Dependency e.mount has not been defined"
	};
}

cmd_errout __shellclose(size_t *buffer, void **persistent){
	// Attach Sym Handle
	char *alias = ((char *)buffer[0]);
	if(!alias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *symcd = getcmd("e.sym");
	if(symcd->persistent){
		// Disable string flag.
		fhandle *fh = resolvedata(alias);
		if(fh){fuloadh(fh);
		}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Handle/Alias does not exist"};}
		// Remove the Symbol.
		for(uint32_t cc = 0; cc < ((symrefbuffer *)symcd->persistent)->len; ++cc){
			if(memcmp(alias, ((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].alias, __min(strlen(((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].alias), strlen(alias)))){
				memcpy(((symrefbuffer *)symcd->persistent)->symrefbuffer + cc, ((symrefbuffer *)symcd->persistent)->symrefbuffer + cc + 1, sizeof(__symref) * (((symrefbuffer *)symcd->persistent)->len - (cc + 1)));
				((symrefbuffer *)symcd->persistent)->len--;
				break;
			}
		}
		return(cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Dependency e.mount has not been defined"};
}

cmd_errout __shellread(size_t *buffer, void **persistent){
	char *inalias = ((char *)buffer[0]), *outalias = ((char *)buffer[1]);
	if(!inalias || !outalias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	size_t pos = buffer[2], nbytes = buffer[3];
	fhandle *fh = resolvedata(inalias);
	if(fh){
		void *data = resolvedata(outalias);
		if(data){free(data);}   data = NULL;
		_fseek(fh, pos);
		if(nbytes == _fread(fh, nbytes, &data)){return (cmd_errout){.errcode = 0, .msg = ""};
		}else{return (cmd_errout){.errcode = cmddescerrtype_eom, .msg = "Could not read all bytes"};}
	}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Output Alias does not exist"};}
}

cmd_errout __shellwrite(size_t *buffer, void **persistent){
	char *inalias = ((char *)buffer[0]), *outalias = ((char *)buffer[1]);
	if(!inalias || !outalias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	size_t pos = buffer[2], nbytes = buffer[3];
	fhandle *fh = resolvedata(outalias);
	if(fh){
		void *data = resolvedata(inalias);
		if(data){
			_fseek(fh, pos);
			if(nbytes == _fwrite(fh, nbytes, data)){return (cmd_errout){.errcode = 0, .msg = ""};
			}else{return (cmd_errout){.errcode = cmddescerrtype_eom, .msg = "Could not Write all bytes"};}
		}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Input Alias does not exist"};}
	}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Output Alias does not exist"};}
}

cmd_errout __shellfwrite(size_t *buffer, void **persistent){
	char *parentpath = ((char *)buffer[0]);
	fhandle *file = ((fhandle *)buffer[1]);
	if(!parentpath || !file){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	size_t pos = buffer[2], nbytes = buffer[3];
	cmddesc *mountcd = getcmd("e.mount");
	FILE *f = fopen(parentpath, "rb+");
	if(f){
		_fseek(file, pos);
		if(!nbytes){fseek(f, 0, SEEK_END);	nbytes = ftell(f);		fseek(f, pos, SEEK_SET);}else{fseek(f, pos, SEEK_SET);}
		void *data = calloc(1, nbytes);
		if(fread(data, 1, nbytes, f) != nbytes){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Could Not read enough Bytes from Parent Filesystem Item"};}
		if(_fwrite(file, nbytes, data) != nbytes){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Could Not write enough Bytes from FrAT Filesystem Item"};}
		free(data);
		fclose(f);
	}
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellfread(size_t *buffer, void **persistent){
	char *parentpath = ((char *)buffer[0]);
	fhandle *file = ((fhandle *)buffer[1]);
	if(!parentpath || !file){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	size_t pos = buffer[2], nbytes = buffer[3];
	FILE *f = fopen(parentpath, "rb+");
	if(f){
		fseek(f, pos, SEEK_SET);		_fseek(file, pos);
		if(!nbytes){nbytes = __fsize(file);}
		void *data = NULL;
		if(_fread(file, nbytes, &data) != nbytes){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Could Not read enough Bytes from FrAT Filesystem Item"};}
		if(fwrite(data, 1, nbytes, f) != nbytes){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Could Not write enough Bytes to Parent Filesystem Item"};}
		fclose(f);
	}
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellsymdump(size_t *buffer, void **persistent){
	char *inalias = ((char *)buffer[0]), *path = ((char *)buffer[1]);
	if(!inalias || !path){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	size_t nbytes = buffer[2];
	FILE *f = fopen(path, "wb+");
	if(f){
		void *data = resolvedata(inalias);
		if(data){fwrite(data, 1, nbytes, f);		fclose(f);		return (cmd_errout){.errcode = 0, .msg = ""};
		}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "The Input Alias does not exist"};}
	}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "The Parent File-System item does not exist"};}
}

cmd_errout __shellsymprint(size_t *buffer, void **persistent){
	char *symbol = ((char *)buffer[0]);
	if(!symbol){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	if(!strncmp(symbol, "..", __min(strlen(symbol), 2))){
		cmddesc *symcd = getcmd("e.sym");
		symrefbuffer *srefb = symcd->persistent;
		if(srefb){
			for(uint32_t cc = 0; cc < srefb->len; ++cc){
				char *format = calloc(32, sizeof(char));
				if(srefb->symrefbuffer[cc].type == symtype_e__string){
					sprintf(format, "[%%u]\t{%%s:\t%%.%llus}", srefb->symrefbuffer[cc].nbytes);
				}else{format = "[%u]\t{%s:\t%s}";}
				dualprintf(logf, stdout, format, cc, srefb->symrefbuffer[cc].alias, ((srefb->symrefbuffer[cc].type == symtype_e__string)? srefb->symrefbuffer[cc].data: "Non-String Data"));
				free(format);
			}
		}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "No Defined Symbols"};}
	}else{
		// 	Bypass resolvedata
		cmddesc *symcd = getcmd("e.sym");
		symrefbuffer *srefb = symcd->persistent;
		if(srefb){
			for(uint32_t cc = 0; cc < srefb->len; ++cc){
				if(memcmp(symbol, srefb->symrefbuffer[cc].alias, __min(strlen(srefb->symrefbuffer[cc].alias), strlen(symbol)))){
					char *format = calloc(32, sizeof(char));
					if(srefb->symrefbuffer[cc].type == symtype_e__string){
						sprintf(format, "[%%u]\t{%%s:\t%%.%llus}", srefb->symrefbuffer[cc].nbytes);
					}else{memcpy(format, "[%u]\t{%s:\t%s}", 14);}
					dualprintf(logf, stdout, format, cc, srefb->symrefbuffer[cc].alias, ((srefb->symrefbuffer[cc].type == symtype_e__string)? srefb->symrefbuffer[cc].data: "Non-String Data"));
					free(format);
					return (cmd_errout){.errcode = 0, .msg = ""};
				}
			}
		}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Symbol does not exist"};}
	}
}

cmd_errout __shellexit(size_t *buffer, void **persistent){
	cmddesc *mountcd = getcmd("e.mount"), *symcd = getcmd("e.sym");
	for(uint32_t cc = 0; cc < ((symrefbuffer *)symcd->persistent)->len; ++cc){
		switch(((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].type){
			case symtype_e__fhandle: {fuloadh(((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].data);		continue;}
			case symtype_e__dirhandle: {fuloaddir(((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].data);	continue;}
			default: {continue;}
		}
	}
	fuloadroot(mountcd->persistent);
	fclose(logf);
	exit(EXIT_SUCCESS);
}

volatile cmddesc commands[] = {
	{
		.cmd = "help", .alias = ",h", .desc = "Print this Info...", .func = __shellhelp,
		.persistent = NULL, .flags = {.flags = {
			{.flag = "-h", .type = cmddescargtype__string, .desc = "The Command to be scanned, Use \"..\" to Print all Info"}, 
			{0}, {0}, {0}, {0}}, .nflags = 1}
	}, {
		.cmd = "e.sym", .alias = ",s", .desc = "Symbol Info, referenced as %<SYMBOLNAME>%", .func = __shellsym,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-%", .type = cmddescargtype__string, .desc = "The Symbol Alias"}, 
				{.flag = "-n", .type = cmddescargtype__qword, .desc = "The Number of Bytes to write"}, 
				{.flag = "-d", .type = cmddescargtype__longdata, .desc = "The Symbol Data"}, 
				{0}, {0}
			}, .nflags = 3}
	}, {
		.cmd = "symdump", .alias = ",sd", .desc = "Dump a Symbol", .func = __shellsymdump,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-%", .type = cmddescargtype__string, .desc = "The Symbol Alias"}, 
				{.flag = "-P", .type = cmddescargtype__string, .desc = "The Output File on the Parent FileSystem"}, {0}, {0}, {0}
			}, .nflags = 2}
	}, {
		.cmd = "symprint", .alias = ",sp", .desc = "Print a Symbol(If it is a String)", .func = __shellsymprint,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-n", .type = cmddescargtype__string, .desc = "The Name of the Symbol to print, a value of \"..\" prints all Symbols."}, 
				{0}, {0}, {0}, {0}
			}, .nflags = 1}
	}, {
		.cmd = "e.disk", .alias = ",u", .desc = "Mount the Disk", .func = __shelldisk,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-pp", .type = cmddescargtype__string, .desc = "The Real File Path on Parent Disk"},
				{.flag = "-pt", .type = cmddescargtype__string, .desc = "<OPTIONAL> The Name of the Partition to be formatted when Mounting fails"},
				{.flag = "-bs", .type = cmddescargtype__dword, .desc = "<OPTIONAL> The Configured Block Size of the Partition to be formatted when Mounting fails"},
				{.flag = "-lb", .type = cmddescargtype__dword, .desc = "<OPTIONAL> The Configured #Log Sectors of the Partition to be formatted when Mounting fails"},
				{.flag = "-v", .type = cmddescargtype__string, .desc = "<OPTIONAL> The Version code of the Partition to be formatted when Mounting fails.\t<In the Format\"<VERMAJOR>:<VERMINOR>\""}
			}, .nflags = 5}
	}, {
		.cmd = "e.mount", .alias = ",m", .desc = "The Mounted Disk FrAT FS, this has no Function and is non-callable", .func = NULL,
		.persistent = NULL, .flags = {.flags = {0}, .nflags = 0}
	}, {
		.cmd = "e.flush", .alias = ",fu", .desc = "Flush all Configuration Data", .func = __shellflush,
		.persistent = NULL, .flags = {.flags = {0}, .nflags = 0}
	}, {
		// Requires e.mount set
		.cmd = "create", .alias = ",cr", .desc = "Create a FileSystem Item...", .func = __shellcreate,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-la", .type = cmddescargtype__string, .desc = "The Create-Args to use for creating a FileSystem Item"}, 
				{.flag = "-p", .type = cmddescargtype__string, .desc = "The Path to the Item"}, 
				{.flag = "-n", .type = cmddescargtype__string, .desc = "The Name of the Item"}
			}, .nflags = 3
		}
	}, {
		.cmd = "delete", .alias = ",d", .desc = "Delete a FileSystem Item...", .func = __shelldelete,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-a", .type = cmddescargtype__string, .desc = "The Path of the item to deleted"}
			}, .nflags = 1
		}
	}, {
		.cmd = "open", .alias = ",o", .desc = "Open an Item", .func = __shellopen,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-la", .type = cmddescargtype__string, .desc = "The Create-Args to use for creating a FileSystem Item"}, 
				{.flag = "-p", .type = cmddescargtype__string, .desc = "The Path of the item to be opened"},
				{.flag = "-a", .type = cmddescargtype__string, .desc = "The Alias of the Handle"}
			}, .nflags = 3
		}
	}, {
		.cmd = "close", .alias = ",cl", .desc = "Open an Item", .func = __shellclose,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-a", .type = cmddescargtype__string, .desc = "The Alias of the Handle"}
			}, .nflags = 1
		}
	}, {
		.cmd = "read", .alias = ",r", .desc = "Read an Item from one Alias to another", .func = __shellread,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-oa", .type = cmddescargtype__string, .desc = "The Output Alias"}, 
				{.flag = "-ia", .type = cmddescargtype__string, .desc = "The Alias of the Item to read from"},
				{.flag = "-p", .type = cmddescargtype__qword, .desc = "The Byte Position to read from"},
				{.flag = "-n", .type = cmddescargtype__qword, .desc = "The #Bytes to read"},
			}, .nflags = 4
		}
	}, {
		.cmd = "write", .alias = ",w", .desc = "Write an Item from one Alias to another", .func = __shellwrite,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-oa", .type = cmddescargtype__string, .desc = "The Input Alias"}, 
				{.flag = "-ip", .type = cmddescargtype__string, .desc = "The Alias of the Item to Write to"},
				{.flag = "-p", .type = cmddescargtype__qword, .desc = "The Byte Position to read from"},
				{.flag = "-n", .type = cmddescargtype__qword, .desc = "The #Bytes to read"},
			}, .nflags = 4
		}
	}, {
		.cmd = "fread", .alias = ",fr", .desc = "Read an Item to the Parent Filesystem from a FrAT Filesystem Item", .func = __shellfread,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-pp", .type = cmddescargtype__string, .desc = "The Path on the Parent Disk's Filesystem to write to"}, 
				{.flag = "-ia", .type = cmddescargtype__string, .desc = "The Alias of the Item to read from"},
				{.flag = "-p", .type = cmddescargtype__qword, .desc = "The Byte Position to read from"},
				{.flag = "-n", .type = cmddescargtype__qword, .desc = "The #Bytes to read <a Value of 0 means \"Read full file\">"},
			}, .nflags = 4
		}
	}, {
		.cmd = "fwrite", .alias = ",fw", .desc = "Write an Item from the Parent Filesystem to a FrAT Filesystem Item", .func = __shellfwrite,
		.persistent = NULL, .flags = {
			.flags = {
				{.flag = "-pp", .type = cmddescargtype__string, .desc = "The Path on the Parent Disk's Filesystem to read from"}, 
				{.flag = "-ia", .type = cmddescargtype__string, .desc = "The Alias of the Item to write to"},
				{.flag = "-p", .type = cmddescargtype__qword, .desc = "The Byte Position to read from"},
				{.flag = "-n", .type = cmddescargtype__qword, .desc = "The #Bytes to read <a Value of 0 means \"Read full file\">"},
			}, .nflags = 4
		}
	}, {
		.cmd = "exit", .alias = ",e", .desc = "Exit the Program", .func = __shellexit,
		.persistent = NULL, .flags = {
			.flags = {{0}, {0}, {0}, {0}, {0}}, .nflags = 0
		}
	}
};
const size_t ncommands = (sizeof(commands) / sizeof(cmddesc));