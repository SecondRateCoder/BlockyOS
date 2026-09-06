#include "shell.h"

symtype_e togglesymtype(symtype_e value, bool set){
	volatile static symtype_e write;
	if(set){write = value;}
	return write;
}

cmddesc *getcmd(char *cmd){
	if(!cmd){return NULL;}
	for(uint64_t cc = 0; cc < ncommands; ++cc){
		if(!memcmp(commands[cc].alias, cmd, __min(strlen(cmd), 8)) || 
			!memcmp(commands[cc].cmd, cmd, __min(strlen(cmd), 8))
		){return (commands + cc);}
	}
	return NULL;
}

void *resolvedata(char *src){
	if(!src){return NULL;}
	cmddesc *symcd = getcmd("e.sym");
	if(!symcd || !symcd->persistent){return NULL;}
	symrefbuffer *buffer = symcd->persistent;
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
	if(!src || !*src){return NULL;}
}

cmd_errout __shellparse(char *input){
	if(!input || !*input){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Empty command"};}
    strtok_t *tokstate = strtok_i(input, " \"", strtok__ForceSameBorderingDelims);
    char *tok = strtok_k(tokstate);
	cmddesc *cd = getcmd(tok);
    if(cd && cd->func){
		if(cd == getcmd("e.sym")){togglesymtype(symtype_e__unknowndata, true);}
        uint64_t *buffer = calloc(cd->flags.nflags, sizeof(uint64_t));
		while(tok = strtok_k(tokstate)){
			for(uint64_t cc = 0; cc < cd->flags.nflags; ++cc){
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
							buffer[cc] = (uint64_t)(_temp ? _temp : temp);
							break;
						} case cmddescargtype__longinteger: {
							char *temp = strtok_ff(tokstate), *_temp = resolvedata(temp);
							buffer[cc] = (uint64_t)(_temp ? _temp : temp);
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

cmd_errout __shellhelp(uint64_t *buffer, void **persistent){
	char *command = ((char *)buffer[0]);
	if(!command){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *cd = getcmd(command);
	if(cd){
		dualprintf(fs_logf, stdout, 
			"\n[%llu]"
			"\n\tCommand: %.8s"
			"\n\tAlias: %.8s"
			"\n\tFlags:",
			((uint64_t)cd - (uint64_t)((void *)commands)) / sizeof(cmddesc), cd->cmd, cd->alias
		);
		for(uint8_t cc_ = 0; cc_ < cd->flags.nflags; ++cc_){
			dualprintf(fs_logf, stdout, 
				"\n\t\t{%.3s\t:\t%s\t:\t%s}",
				cd->flags.flags[cc_].flag,
				flagtypetostr(cd->flags.flags[cc_].type),
				cd->flags.flags[cc_].desc
			);
		}
		dualprintf(fs_logf, stdout, "\n\tDesc: %s", cd->desc);
		return (cmd_errout){.errcode = 0, .msg = ""};
	}else if(!strncmp(command, "..", __min(strlen(command), 2))){
		for(uint64_t cc = 0; cc < ncommands; ++cc){
			dualprintf(fs_logf, stdout, 
				"\n[%llu]"
				"\n\tCommand: %.8s"
				"\n\tAlias: %.8s"
				"\n\tFlags:",
				cc, commands[cc].cmd, commands[cc].alias
			);
			for(uint8_t cc_ = 0; cc_ < commands[cc].flags.nflags; ++cc_){
				dualprintf(fs_logf, stdout, 
					"\n\t\t{%.3s\t:\t%s\t:\t%s}",
					commands[cc].flags.flags[cc_].flag,
					flagtypetostr(commands[cc].flags.flags[cc_].type),
					commands[cc].flags.flags[cc_].desc
				);
			}
			dualprintf(fs_logf, stdout, "\n\tDesc: %s", commands[cc].desc);
		}
		return (cmd_errout){.errcode = 0, .msg = ""};
	}else if(!strncmp(command, "short", __min(strlen(command), 5))){
		for(uint64_t cc = 0; cc < ncommands; ++cc){
			dualprintf(fs_logf, stdout, "\n [%llu]: %.8s(%.8s)\t", cc, commands[cc].cmd, commands[cc].alias);
			for(uint8_t cc_ = 0; cc_ < commands[cc].flags.nflags; ++cc_){
				dualprintf(fs_logf, stdout, " %.3s(%s)", commands[cc].flags.flags[cc_].flag, flagtypetostr(commands[cc].flags.flags[cc_].type));
			}
		}
		return (cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Command does not exist"};
}

cmd_errout __shelldisk(uint64_t *buffer, void **persistent){
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
			if(!confblocksize){confblocksize = __FS_DEFAULTBLOCKSIZE;	dualprintf(fs_logf, stdout, "\nEnabling default Block Size %u", __FS_DEFAULTBLOCKSIZE);}
			if(!conflogblocks){conflogblocks = __FS_DEFAULTLOGSECTORS;	dualprintf(fs_logf, stdout, "\nEnabling default Log Sectors %u", __FS_DEFAULTLOGSECTORS);}
			if(!version){version = __FS_DEFAULTFORMATEDVERSION;			dualprintf(fs_logf, stdout, "\nUsing Default Version: %s", __FS_DEFAULTFORMATEDVERSION);}
			if(partition){
				GPTeNSTR *str = makeGPTeNSTR(partition);
				// Get Version
				strtok_t *tstate = strtok_i(version, " :\"", 0);
                formatpart(path, *str, confblocksize, conflogblocks, 
                    strtoul(strtok_k(tstate), NULL, 10), strtoul(strtok_k(tstate), NULL, 10));
                strtok_d(tstate);
				free(str);
				cd->persistent = fmount(path);
				if(!cd->persistent){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Failed to load Partition"};}
			}else{return (cmd_errout){.errcode  = cmddescerrtype_undefined_setting, .msg = "No Partition target was set."};}
		}
		cmddesc *symcd = getcmd("e.sym");
		togglesymtype(symtype_e__string, true);
		char *diskpath = strdup(path);
		uint64_t symargs[3] = {(uint64_t)"REALDISK", strlen(diskpath), (uint64_t)diskpath};
		symcd->func(symargs, &symcd->persistent);
		free(diskpath);
		return (cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Dependency \"e.mount\" is undefined"};
}

cmd_errout __shellcreate(uint64_t *buffer, void **persistent){
	// type:path:name
	char *type = (char *)(buffer[0]), *path = (char *)(buffer[1]), *name = (char *)(buffer[2]);
	dualprintf(fs_logf, stdout, "\n%p:%s\t%p:%s\t%p:%s\t", type, type, path, path, name, name);
	if(!type || !path || !name){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	char *typedup = strdup(type);
	uint64_t typeLen = strlen(type);
	typedup = realloc(typedup, typeLen + 2);
	typedup[typeLen] = 'c';
	typedup[typeLen + 1] = '\0';
	// Initialise Persistent
	// Use persistent in "e.mount"
	cmddesc *mountcd = getcmd("e.mount");
	if(mountcd){
		togglesymtype(symtype_e__fhandle, true);
		char *npath = strdup(path);
		uint64_t pathLen = strlen(path), nameLen = strlen(name);
		if(npath[pathLen - 1] == PATHSEP || npath[pathLen - 1] == PATHnoSEP){
			npath = realloc(npath, pathLen + nameLen + 1);
			memcpy(npath + pathLen, name, nameLen + 1);
		}else{
			npath = realloc(npath, pathLen + nameLen + 2);
			npath[pathLen] = PATHSEP;
			memcpy(npath + pathLen + 1, name, nameLen + 1);
		}
		dualprintf(fs_logf, stdout, "\n[\t%s]", npath);
		if(mountcd->persistent){fuloadh(floadh(mountcd->persistent, npath, typedup));}
	}else{free(typedup); return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "\nError... Could not set Persistent"};}
	free(typedup);
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellsym(uint64_t *buffer, void **persistent){
	uint64_t nbytes = buffer[1];
	char *alias = (char *)(buffer[0]), *data = (char *)(buffer[2]);
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
		.data = (togglesymtype(0, false) == symtype_e__string ? strdup(data) : data),
		.type = togglesymtype(0, false),
		.nbytes = (togglesymtype(0, false) == symtype_e__string ? strlen(data) : nbytes)
	};
	togglesymtype(symtype_e__string, true);
	snprintf(srb->symrefbuffer[srb->len - 1].alias, strlen(alias) + 5, "%%%s%%", alias);
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellflush(uint64_t *buffer, void **persistent){
	for(uint64_t cc = 0; cc < ncommands; ++cc){
		if((uint64_t)(commands[cc].func) == (uint64_t)__shellsym){
			symrefbuffer *srb = commands[cc].persistent;
			if(srb){
				for(uint32_t index = 0; index < srb->len; ++index){
					free(srb->symrefbuffer[index].alias);
					if(srb->symrefbuffer[index].type == symtype_e__string){
						free(srb->symrefbuffer[index].data);
					}
				}
				free(srb->symrefbuffer);
				free(srb);
			}
			commands[cc].persistent = NULL;
		}else{
			free(commands[cc].persistent);
			commands[cc].persistent = NULL;
		}
	}
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shelldelete(uint64_t *buffer, void **persistent){
	if(!(*buffer)){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	__ffremove(getcmd("e.mount")->persistent, (char *)*buffer);
	return (cmd_errout){.errcode = 0, .msg = ""};
}

cmd_errout __shellopen(uint64_t *buffer, void **persistent){
	// Attach Sym Handle
	char *loadargs = ((char *)buffer[0]), *path = ((char *)buffer[1]), *alias = ((char *)buffer[2]);
	if(!loadargs || !path || !alias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *symcd = getcmd("e.sym"), *mountcd = getcmd("e.mount");
	if(mountcd->persistent){
		// Disable string flag.
		togglesymtype(symtype_e__fhandle, true);
		fhandle *fh = floadh(mountcd->persistent, path, loadargs);
		symcd->func((uint64_t *)((char *[3]){alias, (char *)sizeof(fhandle), (char *)(fh? fh: NULL)}), &symcd->persistent);
		return(cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){
		.errcode = cmddescerrtype_undefined_setting,
		.msg = "Dependency e.mount has not been defined"
	};
}

cmd_errout __shellclose(uint64_t *buffer, void **persistent){
	// Attach Sym Handle
	// char *alias = ((char *)buffer[0]);
	// if(!alias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *symcd = getcmd("e.sym");
	if(symcd->persistent){
		// Disable string flag.
		if(!symcd || !symcd->persistent){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Dependency e.sym has not been defined"};}
		// char tmp[64] = {0};
		// snprintf(tmp, 64, "%%%s%%", alias);
		fhandle *fh = (void *)buffer[0];// (fhandle *)resolvedata(tmp);
		if(fh){fuloadh(fh);}else{
			return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Handle/Alias does not exist"};}
		// Remove the Symbol.
		if(!symcd || !symcd->persistent){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Dependency e.sym has not been defined"};}
		for(uint32_t cc = 0; cc < ((symrefbuffer *)symcd->persistent)->len; ++cc){
			if((uint64_t)fh == (uint64_t)((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].data){
				memcpy(((symrefbuffer *)symcd->persistent)->symrefbuffer + cc, ((symrefbuffer *)symcd->persistent)->symrefbuffer + cc + 1, 
					sizeof(__symref) * (((symrefbuffer *)symcd->persistent)->len - (cc + 1)));
				((symrefbuffer *)symcd->persistent)->len--;
				break;
			}
		}
		return(cmd_errout){.errcode = 0, .msg = ""};
	}
	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Dependency e.mount has not been defined"};
}

cmd_errout __shellread(uint64_t *buffer, void **persistent){
	void *inalias = (void *)buffer[0], *outalias = (void *)buffer[1];
	if(!inalias || !outalias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	uint64_t pos = buffer[2], nbytes = buffer[3]? buffer[3]: __fsize((fhandle *)outalias);
	fhandle *fh = outalias;
	if(fh){
		void *data = inalias, *temp = malloc(nbytes - pos);
		// if(data){free(data);}   data = NULL;
		_fseek(fh, pos);
		if(nbytes == _fread(fh, nbytes, &temp)){
			memcpy(data, temp, nbytes);		free(temp);
			return (cmd_errout){.errcode = 0, .msg = ""};
		}else{return (cmd_errout){.errcode = cmddescerrtype_eom, .msg = "Could not read all bytes"};}
	}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Output Alias does not exist"};}
}

cmd_errout __shellwrite(uint64_t *buffer, void **persistent){
	void *inalias = (void *)buffer[0], *outalias = (void *)buffer[1];
	if(!inalias || !outalias){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	uint64_t pos = buffer[2], nbytes = buffer[3];
	fhandle *fh = outalias;
	if(fh){
		void *data = inalias;
		if(data){
			_fseek(fh, pos);
			if(nbytes == _fwrite(fh, nbytes, data)){return (cmd_errout){.errcode = 0, .msg = ""};
			}else{return (cmd_errout){.errcode = cmddescerrtype_eom, .msg = "Could not Write all bytes"};}
		}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Input Alias does not exist"};}
	}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Output Alias does not exist"};}
}

cmd_errout __shellfwrite(uint64_t *buffer, void **persistent){
	char *parentpath = ((char *)buffer[0]);
	fhandle *file = ((fhandle *)buffer[1]);
	if(!parentpath || !file){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	uint64_t pos = buffer[2], nbytes = buffer[3];
	// cmddesc *mountcd = getcmd("e.mount");
	FILE *f = fopen(parentpath, "rb+");
	if(!f){
		char *_cwd = strdup(getcwd(NULL, 0));
		uint64_t temp = strlen(_cwd);
		_cwd = realloc(_cwd, !(_cwd[temp] == PATHSEP || _cwd[temp] == PATHnoSEP) + temp + strlen(parentpath) + 4);
		if(!(_cwd[temp] == PATHSEP || _cwd[temp] == PATHnoSEP)){if(strcheck(_cwd, PATHSEP)){_cwd[temp] = PATHSEP;}else{_cwd[temp] = PATHnoSEP;}}
		memcpy(_cwd + (_cwd[temp] == PATHSEP || _cwd[temp] == PATHnoSEP) + temp, parentpath, strlen(parentpath) + 1);
		f = fopen(_cwd, "rb+");
		free(_cwd);
	}
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

cmd_errout __shellfread(uint64_t *buffer, void **persistent){
	char *parentpath = ((char *)buffer[0]);
	fhandle *file = ((fhandle *)buffer[1]);
	if(!parentpath || !file){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	uint64_t pos = buffer[2], nbytes = buffer[3];
	FILE *f = fopen(parentpath, "rb+");
	if(!f){
		char *_cwd = strdup(getcwd(NULL, 0));
		uint64_t temp = strlen(_cwd);
		_cwd = realloc(_cwd, (_cwd[temp] == PATHSEP || _cwd[temp] == PATHnoSEP? 0x00: 0x01) + temp + strlen(parentpath));
		if(!(_cwd[temp] == PATHSEP || _cwd[temp] == PATHnoSEP)){_cwd[temp] = PATHSEP;}
		memcpy(_cwd + (_cwd[temp] == PATHSEP || _cwd[temp] == PATHnoSEP? 0x00: 0x01) + temp, parentpath, strlen(parentpath));
		f = fopen(_cwd, "rb+");
		free(_cwd);
	}
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

cmd_errout __shellsymdump(uint64_t *buffer, void **persistent){
	void *inalias = (void *)buffer[0];
	char *path = ((char *)buffer[1]);
	if(!inalias || !path){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	uint64_t nbytes = buffer[2];
	FILE *f = fopen(path, "wb+");
	if(f){
		void *data = inalias;
		if(data){fwrite(data, 1, nbytes, f);		fclose(f);		return (cmd_errout){.errcode = 0, .msg = ""};
		}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "The Input Alias does not exist"};}
	}else{return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "The Parent File-System item does not exist"};}
}

cmd_errout __shellsymprint(uint64_t *buffer, void **persistent){
	char *symbol = ((char *)buffer[0]);
	if(!symbol){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Non-optional arg(s) not set"};}
	cmddesc *symcd = getcmd("e.sym");
	symrefbuffer *srefb = symcd? symcd->persistent : NULL;
	if(!srefb){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "No Defined Symbols"};}
	if(!strncmp(symbol, "..", __min(strlen(symbol), 2))){
		for(uint32_t cc = 0; cc < srefb->len; ++cc){
			char format[64];
			if(srefb->symrefbuffer[cc].type == symtype_e__string){
				snprintf(format, sizeof(format), "\n[%u]\t{%s:\t%.%llus}", cc, srefb->symrefbuffer[cc].alias, srefb->symrefbuffer[cc].nbytes);
				dualprintf(fs_logf, stdout, format, cc, srefb->symrefbuffer[cc].alias, srefb->symrefbuffer[cc].data);
			}else{
				snprintf(format, sizeof(format), "\n[%u]\t{%s:\t%s}", cc, srefb->symrefbuffer[cc].alias, "Non-String Data");
				dualprintf(fs_logf, stdout, format, cc, srefb->symrefbuffer[cc].alias, "Non-String Data");
			}
		}
		return (cmd_errout){.errcode = 0, .msg = ""};
	}
	for(uint32_t cc = 0; cc < srefb->len; ++cc){
		if(!strcmp(symbol, srefb->symrefbuffer[cc].alias) || !strcmp(symbol, srefb->symrefbuffer[cc].alias + 1) || !strncmp(symbol, srefb->symrefbuffer[cc].alias + 1, strlen(symbol))){
			char format[64];
			if(srefb->symrefbuffer[cc].type == symtype_e__string){
				snprintf(format, sizeof(format), "[%u]\t{%s:\t%.%llus}", cc, srefb->symrefbuffer[cc].alias, srefb->symrefbuffer[cc].nbytes);
				dualprintf(fs_logf, stdout, format, cc, srefb->symrefbuffer[cc].alias, srefb->symrefbuffer[cc].data);
			}else{
				snprintf(format, sizeof(format), "[%u]\t{%s:\t%s}", cc, srefb->symrefbuffer[cc].alias, "Non-String Data");
				dualprintf(fs_logf, stdout, format, cc, srefb->symrefbuffer[cc].alias, "Non-String Data");
			}
			return (cmd_errout){.errcode = 0, .msg = ""};
		}
	}
	return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Symbol not found"};
}

cmd_errout __shellexit(uint64_t *buffer, void **persistent){
	cmddesc *mountcd = getcmd("e.mount"), *symcd = getcmd("e.sym");
	if(!mountcd || !symcd || !symcd->persistent){return (cmd_errout){.errcode = cmddescerrtype_undefined_setting, .msg = "Dependency e.mount has not been defined"};}
	for(uint32_t cc = 0; cc < ((symrefbuffer *)symcd->persistent)->len; ++cc){
		switch(((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].type){
			case symtype_e__fhandle: {fuloadh((fhandle *)((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].data);		continue;}
			case symtype_e__dirhandle: {fuloaddir((dirhandle *)((symrefbuffer *)symcd->persistent)->symrefbuffer[cc].data);	continue;}
			default: {continue;}
		}
	}
	fuloadroot(mountcd->persistent);
	fclose(fs_logf);
	exit(EXIT_SUCCESS);
}

cmd_errout __shellsync(uint64_t *buffer, void **persistent){
	printf("\n__FRAT_SYNC__\n");
	fflush(stdout);
	return (cmd_errout){.errcode = 0, .msg = ""};
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
				{.flag = "-ia", .type = cmddescargtype__string, .desc = "The Alias of the Item to Write to"},
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
		.cmd = "sync", .alias = ",sy", .desc = "Synchronize shell output", .func = __shellsync,
		.persistent = NULL, .flags = {.flags = {0}, .nflags = 0}
	}, {
		.cmd = "exit", .alias = ",e", .desc = "Exit the Program", .func = __shellexit,
		.persistent = NULL, .flags = {
			.flags = {{0}, {0}, {0}, {0}, {0}}, .nflags = 0
		}
	}
};
const uint64_t ncommands = (sizeof(commands) / sizeof(cmddesc));