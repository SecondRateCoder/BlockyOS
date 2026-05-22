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
#ifdef __DEBUG__
	printf("\nLoading Partition: %.72s", name);
#endif
	if(checkdisk(ppath)){
		rawenv *re = startup(ppath);
		configureblocksize(512);
		uint8_t *block = readblock(re, 1, 1);
		miniGPT *gpt = (miniGPT *)block;
		GPTentry *ge = (GPTentry *)(readblock(re, gpt->PartEntryLoc, (gpt->nPartEntries * getblocksize()) / sizeof(GPTentry)));
		dispose(re);
		for(uint32_t i = 0; i < gpt->nPartEntries; i++){
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
#ifdef __DEBUG__
	printf("\nFormatting Partition: %.72s", name);
#endif
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
	LBA parttable = gpt->PartEntryLoc;
	GPTentry *ge = (GPTentry *)(readblock(re, parttable, (gpt->nPartEntries * getblocksize()) / sizeof(GPTentry)));
	for(uint32_t i = 0; i < gpt->nPartEntries; i++){
		if(queryfs(re, getblocksize() * ge[i].sLBA)){
			LBA *out = malloc(sizeof(LBA) * 2);
			out[0] = ge[i].sLBA;    out[1] = ge[i].eLBA;
			free(ge);
			return out;
		}
	}
	return NULL;
}

LBA getloc(conf_fsroot *root, fsblock *fb){
	return root->clusterbuffer.clusterLBA + root->clusterbuffer.clusterSectors + (((size_t)fb - (size_t)root->clusterbuffer.clusterMap) / sizeof(fsblock));
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
#ifdef __DEBUG__
	printf("\nMounting FS Root: %s", path);
#endif
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
		conf_fsroot *largeroot = malloc(sizeof(conf_fsroot));
		largeroot->loc = partition[0];
		largeroot->root = fsroot_;
		largeroot->lastClusterAlloc = 0;
		largeroot->logLBA = partition[0] + 1;
		largeroot->clusterbuffer.clusterLBA = partition[0] + 2;
		size_t items = ((partition[1] - partition[0]) / getblocksize()) - ((partition[1] - partition[0]) % getblocksize());
		largeroot->clusterbuffer.clusterSectors = items;
		largeroot->clusterbuffer.clusterSize = (items * getblocksize()) / sizeof(fsblock);
		largeroot->clusterbuffer.clusterMap = readblock(re, partition[0] + 2, items);
		for(size_t i = 0; i < largeroot->clusterbuffer.clusterSize; ++i){
			// Read and verify ROOTS
			fsblock *f = largeroot->clusterbuffer.clusterMap + i;
			if(f->fcode != 0){
				if(flagcheck(f->attr, __fsmetadatacluster) && f->fcode != 0){
					meta_fsblock *temp = readblock(re, largeroot->loc + 2 + largeroot->clusterbuffer.clusterSectors + (((size_t)f - (size_t)largeroot->clusterbuffer.clusterMap) / sizeof(fsblock)), 1);
					if(memcmp(temp->fsig, FRATBLOCKSIG, 8)){
						printf("\nERROR!\nCorrupted FileSystem Root Block\nERASING ENTRY!!");
						memset(temp, 0, 512);
						f->fcode = 0;
						writeblock(re, temp, largeroot->loc + 2 + largeroot->clusterbuffer.clusterSectors + (((size_t)f - (size_t)largeroot->clusterbuffer.clusterMap) / sizeof(fsblock)), 1);
					}
					free(temp);
				}
			}
		}
		dispose(re);
		return largeroot;
	}
}

fsblock *allocatecluster(conf_fsroot *root){
	for(size_t i = root->lastClusterAlloc; i < root->clusterbuffer.clusterSize; ++i){
		if(root->clusterbuffer.clusterMap[i].fcode == 0){root->lastClusterAlloc = i;		return root->clusterbuffer.clusterMap + i;}
	}
	root->lastClusterAlloc = 0;
	return NULL;
}

void __ffremove(conf_fsroot *root, char *path){
	fsblock *fb = __ffind(root, path);
	fb->fcode = 0;
}
void __ffremoveh(conf_fsroot *root, size_t hash){
	hash &= 0x3FFFFFFFFFF;
	fsblock *fb = __ffindh(root, hash);
	fb->fcode = 0;
}

void __ffremovel(conf_fsroot *root, char *path){return __ffremovelh(root, __getfcode(path));}

void __ffremovelh(conf_fsroot *root, size_t hash){
	hash &= 0x3FFFFFFFFFF;
	for(size_t cc = 0; cc < root->clusterbuffer.clusterSize; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash){root->clusterbuffer.clusterMap[cc].fcode = 0;}
	}
}

void __fcreate(conf_fsroot *root, char *path, char *flags){
#ifdef __DEBUG__
	printf("\nCreating File at: ./%s", path);
#endif
	if(__ffind(root, path)){if(strcheck(flags, 'F')){__ffremove(root, path);}else{return;}}
	fsblock *fb = allocatecluster(root);
	if(fb){
		if(strcheck(flags, 'P')){fb->attr |= __fsproxy;}else{
			if(strcheck(flags, 'd')){fb->attr |= __fsdirectory;}
			if(strcheck(flags, 'f') || !flagcheck(fb->attr, __fsfile)){fb->attr |= __fsfile;}
			if(strcheck(flags, 'r') || !flagcheck(fb->attr, __fsreadonly)){fb->attr |= __fsfile;}
		}
		fb->attr |= __fsmetadatacluster;
		fb->fcode = __getfcode(path);
#ifdef __DEBUG__
		printf("\tFcode: %zu", fb->fcode);
#endif
		fb->index = 0;
		dirhandle *dhandle = __fgetparent(root, path);
		if(dhandle && dhandle->file->fcode != fb->fcode){
			__fdiradd(dhandle, fb);
			__fuloaddir(dhandle);
		}
		char *_path = malloc(strlen(path) + 2);
		*_path = '\\';
		memcpy(_path + 1, path, strlen(path) + 1);
		__finit(root, fb, _path);
	}
}

fsblock *__faddr(conf_fsroot *root, fsblock *family){
#ifdef __DEBUG__
	printf("\nAdding FS Table Entry: %zu, Type: %s", family->fcode, (flagcheck(family->attr, __fsdirectory)? "DIRECTORY": "FILE"));
#endif
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 0;
		for(size_t i = 0; i < root->clusterbuffer.clusterSize; ++i){
			if(root->clusterbuffer.clusterMap[i].fcode == family->fcode){fb->index++;}
		}
		fb->attr = family->attr;
		fb->fcode = family->fcode;
		flagunset(fb->attr, __fsmetadatacluster);
		// Clear Block
		void *bl0 = calloc(1, getblocksize());
		memset(bl0, 0, getblocksize());
		LBA loc = getloc(root, fb);
#ifdef __DEBUG__
	printf("\tWriting 0-Block: %zu", loc);
#endif
		rawenv *re = startup(ppath);
		writeblock(re, bl0, loc, 1);
		free(bl0);
		dispose(re);
	}
	return fb;
}

void __finit(conf_fsroot *root, fsblock *fb, char *path){
#ifdef __DEBUG__
	printf("\nInitialising File MetaData: ./%s", path);
#endif
	LBA loc = getloc(root, fb);
	void *block = malloc(getblocksize());
	meta_fsblock *metadata = (meta_fsblock *)block;
	time_t t = time(NULL);
	struct tm *truetime = localtime(&t);
	char *name;
	for(ssize_t i = strlen(path) - 1; i > -1; i--){
		if(i > GPTeNAMELEN){path[i] = '\0';}else{
			if(path[i] == '/' || path[i] == '\\'){
				name = path + strlen(path) - i;  break;
			}else if(!isascii(path[i])){path[i] = 0;}
		}
	}
	*metadata = (meta_fsblock){
		.accessdate = truetime->tm_yday,
		.writedate = 0,
		.accesstime = (truetime->tm_hour * 3600) + (truetime->tm_min * 60) + truetime->tm_sec,
		.writetime = 0,
		.attributes = fb->attr,
		.fcode = (flagcheck(fb->attr, __fsproxy)? 0: fb->fcode),
	};
	memcpy(metadata->fsig, FRATBLOCKSIG, 8);
	flagunset(metadata->attributes, __fsmetadatacluster);
	memset(metadata->name, 0, GPTeNAMELEN);
	memcpy(metadata->name, name, strlen(name));
	rawenv *re = startup(ppath);
	writeblock(re, block, loc, 1);
	dispose(re);
}

void *__fread1(conf_fsroot *root, fsblock *fb, size_t i){
	LBA loc = 0;
	if(i != fb->index){
		fsblock *fb_ = NULL;
		for(size_t cc = 0; cc <root->clusterbuffer.clusterSize; ++cc){
			if(root->clusterbuffer.clusterMap[cc].fcode == fb->fcode){
				if(i == root->clusterbuffer.clusterMap[cc].index){fb_ = root->clusterbuffer.clusterMap + cc;    break;}
			}
		}
		if(fb_ == NULL){return NULL;}
		loc = getloc(root, fb_);
	}else{loc = getloc(root, fb);}
#ifdef __DEBUG__
	printf("\nReading File Block at %zu, Item: %zu.\tRoot: %zu", loc, i, fb->fcode);
#endif
	rawenv *re = startup(ppath);
	void *out = readblock(re, loc, 1);
	dispose(re);
	return out;
}

fsblock *__ffindh(conf_fsroot *root, size_t hash){
	hash &= 0x3FFFFFFFFFF;
	for(size_t cc = 0; cc < root->clusterbuffer.clusterSize; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash && root->clusterbuffer.clusterMap[cc].index == 0){
			return root->clusterbuffer.clusterMap + cc;
		}
	}
	return NULL;
}

fsblock *__ffindhi(conf_fsroot *root, size_t hash, size_t index){
	hash &= 0x3FFFFFFFFFF;
	for(size_t cc = 0; cc < root->clusterbuffer.clusterSize; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash && root->clusterbuffer.clusterMap[cc].index == index){
			return root->clusterbuffer.clusterMap + cc;
		}
	}
	return NULL;
}

fsblock *__ffindi(conf_fsroot *root, char *path, size_t index){return __ffindhi(root, __getfcode(path), index);}

fsblock *__ffind(conf_fsroot *root, char *path){
	size_t hash = __getfcode(path);
	return __ffindh(root, hash);
}

void __fpush1(conf_fsroot *root, fsblock *fb, size_t i, void *buffer){
	LBA loc = 0;
	if(i != fb->index){
		fsblock *fb_ = __ffindhi(root, fb->fcode, i);
		if((fb_ = __ffindhi(root, fb->fcode, i)) == NULL){
			fb_ = __faddr(root, fb);
			if(fb_){
				loc = getloc(root, fb_);
			}else{return;}
		}else{loc = getloc(root, fb_);}
	}else{
		if(flagcheck(fb->attr, __fsmetadatacluster)){
			// Reject the Write
#ifdef __DEBUG__
			printf("Error: Attempted Write to Protected Metadata Cluster");
#endif
			return;
		}else{loc = getloc(root, fb);}
		
	}
#ifdef __DEBUG__
	printf("\nWriting File Block at %zu, Item: %zu.\tRoot: %zu", loc, i, fb->fcode);
#endif
	rawenv *re = startup(ppath);
	writeblock(re, buffer, loc, 1);
	dispose(re);
}

dirhandle *__floaddir(conf_fsroot *root, char *path, char *args){
#ifdef __DEBUG__
	printf("\nMounting Dir [%s] with Args: \"%s\"", path, args);
#endif
	dirhandle *out = malloc(sizeof(dirhandle));
	out->root = root;
	out->path = strdup(path);
	out->file = __ffind(root, path);
	out->dirarray = NULL;
	if(!out->file && strcheck(args, 'c')){
		__fcreate(root, path, args);
		out->file = __ffind(root, path);
		if(!out->file){return NULL;}
	}
	if(flagcheck(out->file->attr, __fsdirectory)){__fdirrefresh(out);
	}else{
		free(out->path);
		free(out);
		return NULL;
	}
	return out;
}

void __fuloaddir(dirhandle *handle){
#ifdef __DEBUG__
	printf("\nUn-Mounting Directory [%s]", handle->path);
#endif
	// Flush root
	rawenv *re = startup(ppath);
	writeblock(re, handle->root->root, handle->root->loc, 1);
	// Flush cluster Array/Buffer
	writeblock(re, handle->root->clusterbuffer.clusterMap, handle->root->clusterbuffer.clusterLBA, handle->root->clusterbuffer.clusterSectors);	
	// Flush Entries
	size_t entry = 1;
	fsblock *f = NULL;
	do{
		f = __ffindhi(handle->root, handle->file->fcode, entry);
		if(f){writeblock(re, handle->dirarray + (getblocksize() * (entry - 1)), handle->root->loc + 2 + handle->root->clusterbuffer.clusterSectors + (((size_t)f - (size_t)handle->root->clusterbuffer.clusterMap) / sizeof(fsblock)), 1);}
		entry++;
	}while(f);
	dispose(re);
	free(handle->dirarray);
	free(handle->path);
	free(handle);
}

void __fdirrefresh(dirhandle *handle){
#ifdef __DEBUG__
	printf("\nRefreshing Dir %s", handle->path);
#endif
	rawenv *re = startup(ppath);
	size_t blockprogress = 0;
	bool exit_ = false;
	do{
		// Find the associated Clusters/Blocks
		fsblock *fb = __ffindi(handle->root, handle->path, blockprogress + 1);
		if(fb){
			// Read off the Block 
			handle->dirarray = realloc(handle->dirarray, getblocksize() * (blockprogress + 1));
			void *temp = readblock(re, getloc(handle->root, fb), 1);
			memcpy(handle->dirarray + (getblocksize() * blockprogress), temp, getblocksize());
			free(temp);
			for(size_t cc = 0; cc < getblocksize() / sizeof(diritem); ++cc){if(handle->dirarray[cc].local == 0){exit_ = true;	break;}}
			blockprogress++;
		}else if(!fb){exit_ = true;}
	}while(!exit_);
	handle->loadedblocks = blockprogress;
	dispose(re);
}

fhandle *fsloadh(conf_fsroot *root, char *path, char *args){
#ifdef __DEBUG__
	printf("\nMounting File [%s] with \"%s\"", path, args);
#endif
	// size_t proxyhash = 0;
	if(strcheck(args, 'c')){__fcreate(root, path, args);}
	// if(strcheck(args, 'P')){
	// 	conf_fsblock *finfo = __freadinfo(root, __ffind(root, path));
	// 	if(!strcheck(args, 'R')){
	// 		if(finfo->fcode){return fsloadh(root, path, args);}
	// 	}
	// }
	fhandle *out = malloc(sizeof(fhandle));
	out->file = __ffind(root, path);
	out->root = root;
	out->progress = 0;
	out->progresslimit = FRAT_PROGLIMIT;
	for(uint8_t cc = 0; cc < (sizeof(out->handlecache) / sizeof(out->handlecache[0])); ++cc){
		out->handlecache[cc].block = NULL;
		out->handlecache[cc].progresstimestamp = -1;
		out->handlecache[cc].rw = 0;
	}
	return out;
}

void fsuloadh(fhandle *handle){
#ifdef __DEBUG__
	printf("\nUn-Mounting File [%zu]", handle->file->fcode);
#endif
	// Flush root
	rawenv *re = startup(ppath);
	writeblock(re, handle->root->root, handle->root->loc, 1);
	// Flush cluster Array/Buffer
	writeblock(re, handle->root->clusterbuffer.clusterMap, handle->root->clusterbuffer.clusterLBA, handle->root->clusterbuffer.clusterSectors);
	// Flush HandleCache
	for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
		if(handle->handlecache[cc].block != NULL){
			fsblock *fb = __ffindhi(handle->root, handle->file->fcode, handle->handlecache[cc].progresstimestamp);
			if(fb){
				writeblock(
					re, handle->handlecache[cc].block, 
					getloc(handle->root, fb), 
					1
				);
			}
			free(handle->handlecache[cc].block);
		}
	}
	free(handle);
	dispose(re);
}


meta_fsblock *__freadinfo(conf_fsroot *root, fsblock *fb){
#ifdef __DEBUG__
	printf("\nReading File Info.\tRoot: %zu", fb->fcode);
#endif
	LBA loc = getloc(root, fb);
	rawenv *re = startup(ppath);
	void *out = readblock(re, loc, 1);
	dispose(re);
	return out;
}

void __fupdatetstamp(conf_fsroot *root, fsblock *file, bool wt){
	rawenv *re = startup(ppath);
	meta_fsblock *finfo = __freadinfo(root, file);
	LBA loc = getloc(root, file);
	time_t t = time(NULL);
	struct tm *truetime = localtime(&t);
	char *name;
	if(wt){finfo->writetime = (truetime->tm_hour * 3600) + (truetime->tm_min * 60) + truetime->tm_sec;		finfo->writedate = truetime->tm_yday;}
	finfo->accesstime = (truetime->tm_hour * 3600) + (truetime->tm_min * 60) + truetime->tm_sec;
	finfo->accessdate = truetime->tm_yday;
	writeblock(re, finfo, loc, 1);
	dispose(re);
}

meta_fsblock *_dreadinfo(dirhandle *handle){return __freadinfo(handle->root, handle->file);}
meta_fsblock *_freadinfo(fhandle *handle){return __freadinfo(handle->root, handle->file);}

void _fseek(fhandle *handle, size_t progress){handle->progress = progress;}
void _fseeko(fhandle *handle, ssize_t progress){handle->progress += progress;}

void _fpush1(fhandle *handle, void *buffer){
	// Update TimeStamp
	__fupdatetstamp(handle->root, handle->file, true);
	if(!flagcheck((__ffindh(handle->root, handle->file->fcode))->attr, __fsreadonly)){
		for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
			if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit || handle->handlecache[cc].progresstimestamp == -1){
				if(handle->handlecache[cc].block){free(handle->handlecache[cc].block);}
				handle->handlecache[cc].block = memdup(buffer, getblocksize());
				handle->handlecache[cc].progresstimestamp = handle->progress;
				handle->handlecache[cc].rw = 0;
				__fpush1(handle->root, handle->file, handle->progress, buffer);
				handle->progress++;
				return;
			}
		}
	}
}

void *_fread1(fhandle *handle){
	// Update TimeStamp
	__fupdatetstamp(handle->root, handle->file, true);
	for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
		if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit || handle->handlecache[cc].progresstimestamp == -1){
			if(handle->handlecache[cc].block){free(handle->handlecache[cc].block);}
			void *buffer = __fread1(handle->root, handle->file, handle->progress);
			handle->handlecache[cc].block = memdup(buffer, getblocksize());
			handle->handlecache[cc].progresstimestamp = handle->progress;
			handle->handlecache[cc].rw = 0;
			handle->progress++;
			return buffer;
		}
	}
	return NULL;
}

dirhandle *__fgetparent(conf_fsroot *root, char *path){
	uint32_t cc = strlen(path);
	char *dup = strdup(path);
	while(dup[cc] == PATHnoSEP || dup[cc] == PATHSEP){cc--;}
	for(; cc > 0; --cc){if(dup[cc] == PATHSEP || dup[cc] == PATHnoSEP){dup[cc] = '\0';	break;}}
	if(cc == 0){return NULL;}
	dirhandle *out = __floaddir(root, dup, "dc");
	free(dup);
	return out;
}

void __fdiradd(dirhandle *dir, fsblock *fb){
#ifdef __DEBUG__
	printf("Adding Entry:%zu to Dir [%s:%zu]", fb->fcode, dir->path, dir->file->fcode);
#endif
	do{
		__fdirrefresh(dir);
		uint32_t cc = 0;
		if(dir->dirarray){
			do{
				if(dir->dirarray[cc].fcode == 0){
					dir->dirarray[cc].attributes = fb->attr;
					dir->dirarray[cc].fcode = fb->fcode;
					dir->dirarray[cc].local = 0;
					return;
				}else if(dir->dirarray[cc].fcode == fb->fcode){
					dir->dirarray[cc].attributes = fb->attr;
					return;
				}else{dir->dirarray[cc].local++;}
				cc++;
			}while(dir->dirarray[cc == 0? cc: (cc - 1)].local != 0);
		}
		__faddr(dir->root, dir->file);
	}while(true);
}

bool __ftest(fhandle *h){
#ifdef __DEBUG__
	printf("\nPerforming File Test. Code: %zu", h->file->fcode);
#endif
	bool out = true;
	void *_block = malloc(getblocksize());
	trng__(_block, getblocksize());
	_fseek(h, 1);
	_fpush1(h, _block);
	_fseeko(h, -1);
	void *__block = _fread1(h);
	if(__block){
		printf("\n");
		for(size_t cc = 0; cc < getblocksize(); ++cc){
			if(((uint8_t *)_block)[cc] != ((uint8_t *)__block)[cc]){
				out = false;
#ifdef __DEBUG__
				printf("✖ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
			}else{
				printf("✔ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
#endif
			}
		}
		// out = !memcmp(_block, __block, getblocksize());
	   free(__block);
	}
	free(_block);
	return out;
}

unhandle *__fdirlist(dirhandle *dir, size_t *index){
	__fdirrefresh(dir);
	if((((*index) * sizeof(diritem)) / getblocksize()) < dir->loadedblocks){
		diritem *ditem = dir->dirarray + (*index);
		(*index)++;
		fsblock *fb = __ffindh(dir->root, ditem->fcode);
		if(fb){
			if(fb->fcode == ditem->fcode && (fb->fcode != 0)){
				meta_fsblock *finfo = __freadinfo(dir->root, fb);
				char *temp = strdup(dir->path);
				temp = realloc(temp, strlen(temp) + strlen(finfo->name) + 2);
				memcpy(temp + strlen(dir->path) + (*finfo->name != '/'), finfo->name, strlen(finfo->name) + 1);
				temp[strlen(dir->path)] = PATHSEP;
				free(finfo);
				unhandle *out = malloc(sizeof(unhandle));
				if((out->dir = flagcheck(fb->attr, __fsdirectory))){
					out->dhandle_ = __floaddir(dir->root, temp, "");
				}else{out->fhandle_ = fsloadh(dir->root, temp, "f");}
				free(temp);
				return out;
			}else{
				// Remove Entry
				ditem->fcode = 0;
				return NULL;
			}
		}else{
			// Repair The Entry
		}
	}
	return NULL;
}

dirrunner *__dirr_init(dirhandle *handle, char *patternmatcher){
	if(!handle){return NULL;}
	dirrunner *dr = calloc(1, sizeof(dirrunner));
	if(!dr){return NULL;}
	dr->dir = handle;
	dr->pattmatcher = patternmatcher;
	dr->stack_size = 8;
	dr->stack_depth = 1;
	dr->dir_stack = calloc(dr->stack_size, sizeof(dirhandle*));
	dr->idx_stack = calloc(dr->stack_size, sizeof(size_t));
	if(!dr->dir_stack || !dr->idx_stack){
		free(dr->dir_stack);
		free(dr->idx_stack);
		free(dr);
		return NULL;
	}
	dr->dir_stack[0] = handle;
	dr->idx_stack[0] = 0;
	return dr;
}

static bool __dirr_matches(dirrunner *dr, unhandle *item){
	meta_fsblock *info = item->dir ? _dreadinfo(item->dhandle_) : _freadinfo(item->fhandle_);
	if(!info){return false;}
	bool match = __pattmatch(dr->pattmatcher, info->name);
	free(info);
	return match;
}

static bool __dirr_push(dirrunner *dr, dirhandle *handle){
	if(dr->stack_depth == dr->stack_size){
		size_t new_size = dr->stack_size ? dr->stack_size * 2 : 8;
		dr->dir_stack = realloc(dr->dir_stack, new_size * sizeof(dirhandle*));
		dr->idx_stack = realloc(dr->idx_stack, new_size * sizeof(size_t));
		if(!dr->dir_stack || !dr->idx_stack) return false;
		dr->stack_size = new_size;
	}
	dr->dir_stack[dr->stack_depth] = handle;
	dr->idx_stack[dr->stack_depth] = 0;
	dr->stack_depth++;
	return true;
}

static dirhandle *__dirr_pop(dirrunner *dr){
	if(dr->stack_depth == 0) return NULL;
	dr->stack_depth--;
	dirhandle *out = dr->dir_stack[dr->stack_depth];
	dr->dir_stack[dr->stack_depth] = NULL;
	return out;
}

void __dirr_free(dirrunner *dr){
	if(!dr){return;}
	for(size_t cc = 0; cc < dr->stack_depth; ++cc){
		if(dr->dir_stack[cc]){if(cc > 0){__fuloaddir(dr->dir_stack[cc]);}}
	}
	free(dr->dir_stack);
	free(dr->idx_stack);
	free(dr);
}

unhandle *__dirr(dirrunner *dr){
	if(!dr || dr->stack_depth == 0){return NULL;}
	while(dr->stack_depth > 0){
		size_t top = dr->stack_depth - 1;
		dirhandle *current = dr->dir_stack[top];
		size_t idx = dr->idx_stack[top];
		unhandle *item = __fdirlist(current, &idx);
		dr->idx_stack[top] = idx;

		if(!item){
			if(top > 0){__fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_){
			if(top > 0){__fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_->file){
			if(top > 0){__fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}

		bool matches = __dirr_matches(dr, item);
		if(item->dir){
			if(!__dirr_push(dr, item->dhandle_)){
				free(item);
				return NULL;
			}
		}
		if(matches){return item;}
		free(item);
	}

	return NULL;
}

void __fprint_info(meta_fsblock *finfo){
	printf(
		"\nFile Info:"
		"\nSignature: %.8s"
		"\nVersion: %zu"
		"\nfCode: %zu"
		"\nAttributes:"
		"\n\tIs Directory: [%s]"
		"\n\tIs File: [%s]"
		"\n\tIs Readonly: [%s]"
		"\nName: %.32s"
		"\nAccess Time: %u:%u:%u"
		"\nWrite Time: %u:%u:%u"
		"\nAccess Date: %u"
		"\nWrite Date: %u",
		finfo->fsig, finfo->headerversion, (size_t)finfo->fcode,
		(flagcheck(finfo->attributes, __fsdirectory) ? "TRUE": "FALSE"), (flagcheck(finfo->attributes, __fsfile) ? "TRUE": "FALSE"), 
		(flagcheck(finfo->attributes, __fsreadonly) ? "TRUE": "FALSE"), 
		finfo->name, 
		(uint32_t)(finfo->accesstime / 3600), 
		(uint32_t)((finfo->accesstime % 3600) / 60),
		(uint32_t)(finfo->accesstime % 60), 
		(uint32_t)(finfo->writetime / 3600), 
		(uint32_t)((finfo->writetime % 3600) / 60),
		(uint32_t)(finfo->writetime % 60), 
		finfo->accessdate, finfo->writedate
	);
}