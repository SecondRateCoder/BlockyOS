#include "frat.h"

char *ppath;

bool checkdisk(char *path){
	// Check the GPT for a Disk.
	bool out = 0;
	ppath = strdup(path);
	rawenv *re = startup(path);
	configureblocksize(512);
	uint8_t *block = readblock(re, 1, 1);
	miniGPT *gpt = (miniGPT *)block;
	if(!memcmp(gpt->sig, "EFI PART", 8)){out = true;}else{out = false;}
	free(block);    dispose(re);
	return out;
}

GPTeNSTR *makeGPTeNSTR(char *str){
	GPTeNSTR *out = calloc(sizeof(GPTeNSTR), 1);
	for(size_t cc = 0; cc < __min(strlen(str), GPTeNAMELEN); ++cc){
		(*out)[cc] = str[cc];
	}
	return out;
}

LBA *loadpart(GPTeNSTR name){
	if(checkdisk(ppath)){
		rawenv *re = startup(ppath);
		configureblocksize(512);
		uint8_t *block = readblock(re, 1, 1);
		miniGPT *gpt = (miniGPT *)block;
		GPTentry *ge = (GPTentry *)(readblock(re, gpt->GPTarray, (gpt->partEntries * getblocksize()) / sizeof(GPTentry)));
		dispose(re);
		for(uint32_t i = 0; i < gpt->partEntries; i++){
			if(!memcmp(name, ge[i].name, GPTeNAMESIZE)){
				LBA *out = malloc(sizeof(LBA) * 2);
				out[0] = ge[i].sLBA;    out[1] = ge[i].eLBA;
				free(ge);
				return out;
			}
		}
	}
}

void formatpart(GPTeNSTR name){
	LBA *part = loadpart(name);
	void *block = malloc(getblocksize());
	*((fsroot *)block) = (fsroot){
		.confBlockSize = getblocksize(),
		.confClusterSize = (part[1] - part[0]) * 8,
		.verCode = {0, 1},
		.signature = FRATSIG,
	};
	// Reset all Info
	rawenv *re = startup(ppath);
	writeblock(re, block, part[0], 1);
	memset(block, 0, getblocksize());
	// Clear Log
	writeblock(re, block, part[0] + 1, 1);
	// Clear ClusterMap
	block = realloc(block, (part[1] - part[0]) * 8);
	memset(block, 0, (part[1] - part[0]) * 8);
	writeblock(re, block, part[0] + 2, 1);
	dispose(re);
	free(block);
}

LBA *queryparttablefs(miniGPT *gpt, rawenv *re){
	// Query all Partitions for the FileSystem.
	LBA parttable = gpt->GPTarray;
	GPTentry *ge = (GPTentry *)(readblock(re, parttable, (gpt->partEntries * getblocksize()) / sizeof(GPTentry)));
	for(uint32_t i = 0; i < gpt->partEntries; i++){
		if(queryfs(re, getblocksize() * ge[i].sLBA)){
			LBA *out = malloc(sizeof(LBA) * 2);
			out[0] = ge[i].sLBA;    out[1] = ge[i].eLBA;
			free(ge);
			return out;
		}
	}
	return NULL;
}

bool queryfs(rawenv *re, LBA base){
	// Check that a FileSystem exists at the bytebase.
	bool out = 0;
	uint8_t *block = readblock(re, base / getblocksize(), 3);
	fsroot *fr = (fsroot *)block;
	if(!memcmp(fr->signature, FRATSIG, 22)){out = true;}else{out = false;}
	free(block);
	return out;
}

conf_fsroot *fmount(char *path){
	LBA *partition;
	if(checkdisk(path)){
		// Get the LBA Info for a Partition
		rawenv *re = startup(path);
		void *block = readblock(re, 1, 1);
		miniGPT *gpt = (miniGPT *)block;
		if((partition = queryparttablefs(gpt, re))){
			free(gpt);
		}else{free(gpt);    return NULL;}
		// Generate the fsroot
		block = readblock(re, partition[0], 1);
		fsroot *fsroot_ = (fsroot *)block;
		configureblocksize(fsroot_->confBlockSize);
		conf_fsroot *largeroot = malloc(sizeof(conf_fsblock));
		largeroot->loc = partition[0];
		largeroot->root = fsroot_;
		largeroot->lastClusterAlloc = 0;
		largeroot->logLBA = partition[0] + 1;
		largeroot->clusterBuffer.clusterLBA = partition[0] + 2;
		size_t items = ((partition[1] - partition[0]) / getblocksize()) - ((partition[1] - partition[0]) % getblocksize());
		largeroot->clusterBuffer.clusterSize = (items * getblocksize()) / sizeof(fsblock);
		largeroot->clusterBuffer.fs = readblock(re, partition[0] + 2, items);
		for(size_t i = 0; i < largeroot->clusterBuffer.clusterSize; ++i){
			// Read and verify ROOTS
			fsblock f = largeroot->clusterBuffer.fs[i];
			if(f.fcode != 0){
				conf_fsblock *temp = readblock(re, i + partition[0] + 2, 1);
				if(memcmp(temp->fsig, FRATBLOCKSIG, 8)){
					printf("ERROR!\nCorrupted FileSystem Root Block");
					memset(temp, 0, 512);
					writeblock(re, temp, i + partition[0] + 2, 1);
				}
				free(temp);
			}
		}
		dispose(re);
		return largeroot;
	}
}

fsblock *allocatecluster(conf_fsroot *root){
	for(size_t i = root->lastClusterAlloc; i < root->clusterBuffer.clusterSize; ++i){
		if(root->clusterBuffer.fs[i].fcode == 0){root->lastClusterAlloc = i;		return root->clusterBuffer.fs + i;}
	}
	root->lastClusterAlloc = 0;
	return NULL;
}

void __fcreate(conf_fsroot *root, char *path, char *flags){
	fsblock *fb = allocatecluster(root);
	if(fb){
		if(strcheck(flags, 'd')){fb->attr |= __fsdirectory;}
		if(strcheck(flags, 'f') || !flagcheck(fb->attr, __fsfile)){fb->attr |= __fsfile;}
		if(strcheck(flags, 'r') || !flagcheck(fb->attr, __fsreadonly)){fb->attr |= __fsfile;}
		fb->attr |= __fsmetadatacluster;
		size_t hash = __getfcode(path);
		fb->fcode = hash;
		fb->logalias = 0;
		fb->index = 0;
		dirhandle *dhandle = __fgetparent(root, path);
		if(dhandle){
			for(uint32_t cc = 0; dhandle->dirarray[cc].local != 0; ++cc){
				if(dhandle->dirarray[cc].fcode == 0){
					dhandle->dirarray[cc].fcode = hash;
					dhandle->dirarray[cc].attributes = fb->attr;
				}
			}
			fuloaddir(dhandle);
		}
		char *_path = malloc(strlen(path) + 1);
		*_path = '\\';
		memcpy(_path + 1, path, strlen(path));
		__finit(root, fb, _path);
	}
}

fsblock *__faddr(conf_fsroot *root, fsblock *family){
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 0;
		for(size_t i = 0; i < root->clusterBuffer.clusterSize; ++i){
			if(root->clusterBuffer.fs[i].fcode == family->fcode){fb->index++;}
		}
		fb->attr = family->attr;
		flagunset(fb->attr, __fsmetadatacluster);
	}
	return fb;
}

void __fadd(conf_fsroot *root, fsblock *family){
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 1;
		for(size_t i = 0; i < root->clusterBuffer.clusterSize; ++i){
			if(root->clusterBuffer.fs[i].fcode == family->fcode){fb->index++;}
		}
		fb->attr = family->attr;
		flagunset(fb->attr, __fsmetadatacluster);
	}
}

void __finit(conf_fsroot *root, fsblock *fb, char *path){
	LBA loc = root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)fb - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
	void *block = malloc(getblocksize());
	conf_fsblock *metadata = (conf_fsblock *)block;
	time_t t = time(NULL);
	struct tm *truetime = localtime(&t);
	char *name;
	for(ssize_t i = strlen(path) - 1; i > -1; i--){
		if(path[i] == '/' || path[i] == '\\'){
			name = path - i;  break;
		}else if(!isascii(path[i])){path[i] = 0;}
	}
	*metadata = (conf_fsblock){
		.accessdate = truetime->tm_yday,
		.writedate = 0,
		.accesstime = (truetime->tm_hour * 3600) + (truetime->tm_min * 60) + truetime->tm_sec,
		.writetime = 0,
		.attributes = fb->attr,
		.fcode = fb->fcode,
		.fsig = FRATBLOCKSIG,
	};
	memcpy(metadata->name, name, GPTeNAMELEN);
	rawenv *re = startup(ppath);
	writeblock(re, block, loc, 1);
	dispose(re);
}

void *__fread1(conf_fsroot *root, fsblock *fb, size_t i){
	LBA loc = 0;
	if(i != fb->index){
		fsblock *fb_ = NULL;
		for(size_t cc = 0; cc <root->clusterBuffer.clusterSize; ++cc){
			if(root->clusterBuffer.fs[cc].fcode == fb->fcode){
				if(i == root->clusterBuffer.fs[cc].index){fb_ = root->clusterBuffer.fs + cc;    break;}
			}
		}
		if(fb_ == NULL){return NULL;}
		loc = root->clusterBuffer.clusterLBA + root->clusterBuffer.clusterSize + (((size_t)fb_ - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
	}
	rawenv *re = startup(ppath);
	void *out = readblock(re, loc, 1);
	dispose(re);
	return out;
}

fsblock *__ffind(conf_fsroot *root, char *path){
	size_t hash = __getfcode(path);
	for(size_t cc = 0; cc < root->clusterBuffer.clusterSize; ++cc){
		if(root->clusterBuffer.fs[cc].fcode == hash && root->clusterBuffer.fs[cc].index == 0){
			return root->clusterBuffer.fs + cc;
		}
	}
	return NULL;
}

void __fpush1(conf_fsroot *root, fsblock *fb, size_t i, void *buffer){
	LBA loc = 0;
	if(i != fb->index){
		fsblock *fb_ = NULL;
		for(size_t cc = 0; cc <root->clusterBuffer.clusterSize; ++cc){
			if(root->clusterBuffer.fs[cc].fcode == fb->fcode){
				if(i == root->clusterBuffer.fs[cc].index){fb_ = root->clusterBuffer.fs + cc;    break;}
			}
		}
		if(fb_ == NULL){
			fb_ = __faddr(root, fb);
			if(fb_){
				loc = root->clusterBuffer.clusterLBA + root->clusterBuffer.clusterSize + (((size_t)fb_ - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
			}else{return;}
		}
	}
	rawenv *re = startup(ppath);
	writeblock(re, buffer, loc, 1);
	dispose(re);
}

dirhandle *__floaddir(conf_fsroot *root, char *path, char *args){
	dirhandle *out = malloc(sizeof(dirhandle));
	out->root = root;
	out->path = strdup(path);
	out->current = __ffind(root, path);
	if(flagcheck(out->current->attr, __fsdirectory)){
		rawenv *re = startup(ppath);
		size_t progress = (getblocksize() / sizeof(diritem)) - 1;
		do{
			out->dirarray = (diritem *)readblock(re, root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)out->current - (size_t)root->clusterBuffer.fs) / sizeof(fsblock)), 1);
			progress += (getblocksize() / sizeof(diritem)) - 1;
		}while(out->dirarray[progress].local != 0);
		dispose(re);
	}else{
		free(out);
		return NULL;
	}
	return out;
}

void __fuloaddir(dirhandle *handle){
	// Flush root
	rawenv *re = startup(ppath);
	writeblock(re, (void *)handle->root->root, handle->root->loc, 1);
	// Flush cluster Array/Buffer
	writeblock(re, (void *)handle->root->clusterBuffer.fs, handle->root->clusterBuffer.clusterLBA, handle->root->clusterBuffer.clusterSize);
	
}

fsblock *__fgetitem_s(dirhandle *dir, char *name){
	char *fullpath = strdup(dir->path);
	fullpath = realloc(fullpath, strlen(dir->path) + strlen(name));
	fullpath[strlen(dir->path)] = '/';
	memcpy(fullpath + strlen(dir->path) - 1, name, strlen(name));
	fsblock *out = __ffind(dir->root, fullpath);
	free(fullpath);
	return out;
}

unhandle *__fsearchitem(dirhandle *dir, char *name){
	char *fullpath = strdup(dir->path);
	fullpath = realloc(fullpath, strlen(dir->path) + strlen(name));
	fullpath[strlen(dir->path)] = '/';
	memcpy(fullpath + strlen(dir->path) - 1, name, strlen(name));
	size_t hash = __getfcode(fullpath);
	for(uint32_t cc = 0; dir->dirarray[cc].local != 0; ++cc){
		if(dir->dirarray[cc].fcode == hash){
			unhandle *out = malloc(sizeof(unhandle));
			out->dir = flagcheck(dir->dirarray[cc].attributes, __fsdirectory);
			if(out->dir){out->dhandle_ = __floaddir(dir->root, fullpath, "");
			}else{out->fhandle_ = fsloadh(dir->root, fullpath, "");}
			free(fullpath);
			return out;
		}
	}
	return NULL;
}

fhandle *fsloadh(conf_fsroot *root, char *path, char *args){
	if(strcheck(args, 'c')){__fcreate(root, path, args);}
	fhandle *out = malloc(sizeof(fhandle));
	out->file = __ffind(root, path);
	out->root = root;
	out->progress = 0;
	out->progresslimit = 2;
	for(uint8_t cc = 0; cc < (sizeof(out->handlecache) / sizeof(out->handlecache[0])); ++cc){
		out->handlecache[cc].block = NULL;
		out->handlecache[cc].progresstimestamp = -1;
		out->handlecache[cc].rw = 0;
	}
	return out;
}

void fsuloadh(fhandle *handle){
	// Flush root
	rawenv *re = startup(ppath);
	writeblock(re, (void *)handle->root->root, handle->root->loc, 1);
	// Flush cluster Array/Buffer
	writeblock(re, (void *)handle->root->clusterBuffer.fs, handle->root->clusterBuffer.clusterLBA, handle->root->clusterBuffer.clusterSize);
	// Erase HandleCache
	for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
		if(handle->handlecache[cc].block != NULL){
			writeblock(re, handle->handlecache[cc].block, handle->handlecache[cc].progresstimestamp, 1);
			free(handle->handlecache[cc].block);
		}
	}
	free(handle);
	dispose(re);
}

conf_fsblock *__freadinfo(conf_fsroot *root, fsblock *fb){
	LBA loc = root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)fb - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
	rawenv *re = startup(ppath);
	void *out = readblock(re, loc, 1);
	dispose(re);
	return out;
}

void _fseek(fhandle *handle, size_t progress){handle->progress = progress;}

void _fpush1(fhandle *handle, void *buffer){
	for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
		if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit || handle->handlecache[cc].progresstimestamp == -1){
			if(handle->handlecache[cc].block){free(handle->handlecache[cc].block);}
			handle->progress++;
			handle->handlecache[cc].block = memdup(buffer, getblocksize());
			handle->handlecache[cc].progresstimestamp = handle->progress;
			handle->handlecache[cc].rw = 0;
			__fpush1(handle->root, handle->file, handle->progress, buffer);
			handle->progress++;
			return;
		}
	}
}

void *_fread1(fhandle *handle){
	for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
		if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit || handle->handlecache[cc].progresstimestamp == -1){
			if(handle->handlecache[cc].block){free(handle->handlecache[cc].block);}
			void *buffer = __fread1(handle->root, handle->file, handle->progress);
			handle->handlecache[cc].block = memdup(buffer, getblocksize());
			handle->handlecache[cc].progresstimestamp = handle->progress;
			handle->handlecache[cc].rw = 0;
			void *out = __fread1(handle->root, handle->file, handle->progress);
			handle->progress++;
			return out;
		}
	}
	return NULL;
}

dirhandle *__fgetparent(conf_fsroot *root, char *path){
	for(uint32_t cc = strlen(path); cc > 0; --cc){if(path[cc] == PATHSEP || path[cc] == PATHnoSEP){path[cc] = '\0';}}
	return __floaddir(root, path, "d");
}

void __fdiradd(dirhandle *dir, fsblock *fb){
	bool full_ = true;
	uint32_t cc = 0;
	for(; dir->dirarray[cc].local != 0; ++cc){
		if(dir->dirarray[cc].fcode == 0){
			dir->dirarray[cc].attributes = fb->attr;
			dir->dirarray[cc].fcode = fb->fcode;
			dir->dirarray[cc].local = 0;
			full_ = false;
		}else{dir->dirarray[cc].local++;}
	}
	if(full_){
		fsblock *fb_ = allocatecluster(dir->root);
		*fb_ = (fsblock){
			.attr = dir->current->attr && ~__fsmetadatacluster,
			.fcode = dir->current->fcode,
			.index = cc,
			.logalias = 0
		};
		return __fdiradd(dir, fb);
	}
}

bool __ftest(fhandle *h){
	bool out = false;
	void *_block = malloc(getblocksize());
	// printf("\n%u", getblocksize());
	// for(uint32_t cc = 0; cc < getblocksize(); ++cc){printf("%" PRIu8 "    ;", ((uint8_t *)_block)[cc]);}
	_fpush1(h, _block);
	_fseek(h, 1);
	void *__block = _fread1(h);
	if(__block){
		out = !memcmp(_block, __block, getblocksize());
	   free(__block);
	}
	free(_block);
	return out;
}