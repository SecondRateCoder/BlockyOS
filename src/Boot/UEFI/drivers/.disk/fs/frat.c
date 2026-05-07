#include "frat.h"

BOOLEAN checkdisk(EFI_HANDLE Image){
#ifdef __DEBUG__
	Print(L"\nVerifying Disk GPT");
#endif
	bool out = false;
	UINT32 mID;
	if(!EFI_ERROR(getDriveMediaID(Image, &mID))){
		rawenv re = startup(mID, __FS_DEFAULTBLOCKSIZE);
		uint8_t *block = readblock(re, GPT_LBA, 1);
		miniGPT *gpt = (miniGPT *)block;
#ifdef __DEBUG__
		char dup[9];
		__memcpy(dup, gpt->sig, 8);
		dup[8] = 0;
		Print(
			L"\nsig: %a"
			L"\tRev: %u",
			dup, gpt->rev
		);
#endif
		if(!__memcmp(gpt->sig, "EFI PART", 8)){out = TRUE;}else{out = FALSE;}
		FreePool(block);
		dispose(re);
	}
#ifdef __DEBUG__
	Print(L"\nGPT Exists? %a", ((out == true)? "TRUE": "FALSE"));
#endif
	return out;
}

LBA *loadpart(EFI_HANDLE Image, UINT32 MediaID, GPTeNSTR name){
#ifdef __DEBUG__
	Print(L"\nLoading Partition: %.72s", name);
#endif
	if(checkdisk(Image)){
		rawenv re = startup(MediaID, __FS_DEFAULTBLOCKSIZE);
		setblocksize(re, 512);
		uint8_t *block = readblock(re, 1, 1);
		miniGPT *gpt = (miniGPT *)block;
		GPTentry *ge = (GPTentry *)(readblock(re, gpt->GPTarray, (gpt->partEntries * getblocksize(re)) / sizeof(GPTentry)));
		dispose(re);
		for(uint32_t i = 0; i < gpt->partEntries; i++){
			if(!__memcmp(name, ge[i].name, GPTeNAMESIZE)){
				LBA *out = AllocatePool(sizeof(LBA) * 2);
				out[0] = ge[i].sLBA;    out[1] = ge[i].eLBA;
				FreePool(gpt);
				FreePool(ge);
#ifdef __DEBUG__
				Print(L"\nFound Partition: %llu:%llu", out[0], out[1]);
#endif
				return out;
			}
		}
		FreePool(gpt);
		FreePool(ge);
	}
#ifdef __DEBUG__
	Print(L"\nFound No Partition named: %s", name);
#endif
	return NULL;
}

void formatpart(EFI_HANDLE Image, UINT32 MediaID, GPTeNSTR name){
#ifdef __DEBUG__
	Print(L"\nFormatting Partition: %.72s", name);
#endif
	LBA *part = loadpart(Image, MediaID, name);
	if(!part){return;}
	void *block = AllocatePool(__FS_DEFAULTBLOCKSIZE);
	*((fsroot *)block) = (fsroot){
		.confBlockSize = __FS_DEFAULTBLOCKSIZE,
		.confClusterSize = (part[1] - part[0]) * 8,
		.verCode = {0, 1},
		.signature = FRATSIG,
	};
	// Reset all Info
	rawenv re = startup(MediaID, __FS_DEFAULTBLOCKSIZE);
	writeblock(re, block, part[0], 1);
	__memset(block, 0, getblocksize(re));
	// Clear Log
	writeblock(re, block, part[0] + 1, 1);
	// Clear ClusterMap
	block = __realloc(block,__FS_DEFAULTBLOCKSIZE, (part[1] - part[0]) * 8);
	__memset(block, 0, (part[1] - part[0]) * 8);
	writeblock(re, block, part[0] + 2, 1);
	dispose(re);
	FreePool(block);
}

LBA *queryparttablefs(miniGPT *gpt, rawenv re){
#ifdef __DEBUG__
	Print(L"\nQuerying Part Table");
#endif
	// Query all Partitions for the FileSystem.
	LBA parttable = gpt->GPTarray;
	GPTentry *ge = (GPTentry *)(readblock(re, parttable, (gpt->partEntries * sizeof(GPTentry)) / getblocksize(re)));
	for(uint32_t i = 0; i < gpt->partEntries; i++){
		if(queryfs(re, getblocksize(re) * ge[i].sLBA)){
			LBA *out = AllocatePool(sizeof(LBA) * 2);
			out[0] = ge[i].sLBA;    out[1] = ge[i].eLBA;
			FreePool(ge);
			return out;
		}
	}
	FreePool(ge);
	return NULL;
}

BOOLEAN queryfs(rawenv re, LBA base){
#ifdef __DEBUG__
	Print(L"\nQuerying FS at %llu", base);
#endif
	// Check that a FileSystem exists at the bytebase.
	BOOLEAN out = 0;
	uint8_t *block = readblock(re, base / getblocksize(re), 3);
	fsroot *fr = (fsroot *)block;
	if(!__memcmp(fr->signature, FRATSIG, 22)){out = TRUE;}else{out = FALSE;}
	FreePool(block);
	return out;
}

conf_fsroot *fmount(EFI_HANDLE Image, UINT32 MediaID){
#ifdef __DEBUG__
	Print(L"\nMounting FS Root: %u", MediaID);
#endif
	LBA *partition;
	if(checkdisk(Image)){
		// Get the LBA Info for a Partition
		rawenv re = startup(MediaID, __FS_DEFAULTBLOCKSIZE);
		void *block = readblock(re, GPT_LBA, GPT_BLOCKS(getblocksize(re)));
		miniGPT *gpt = (miniGPT *)block;
		if((partition = queryparttablefs(gpt, re))){
			FreePool(gpt);
		}else{FreePool(gpt);    return NULL;}
#ifdef __DEBUG__
		Print(L"\nFound Partition: %ullu:%llu", partition[0], partition[1]);
#endif
		// Generate the fsroot
		block = readblock(re, partition[0], 1);
		fsroot *fsroot_ = (fsroot *)block;
		setblocksize(re, fsroot_->confBlockSize);
		conf_fsroot *largeroot = AllocatePool(sizeof(conf_fsroot));
		size_t items = ((partition[1] - partition[0]) / getblocksize(re)) - ((partition[1] - partition[0]) % getblocksize(re));
		*largeroot = (conf_fsroot){
#ifdef __FRAT_USEPARTITION_PROTOCOL__
			.loc = partition[0],
#else
			.loc = 0,
#endif
			.root = fsroot_,
			.lastClusterAlloc = 0,
			.logLBA = partition[0] + 1,
			.clusterBuffer = {
				.clusterLBA = partition[0] + 2,
				.clusterSectors = items,
				.clusterSize = (items * getblocksize(re)) / sizeof(fsblock),
				.fs = readblock(re, partition[0] + 2, items)
			},
			.MediaID = MediaID,
			.Image = Image
		};
#ifdef __DEBUG__
		Print(L"Verifying FS Root Items");
		DisableVerbose(re);
#endif
		for(size_t i = 0; i < largeroot->clusterBuffer.clusterSize; ++i){
			// Read and verify ROOTS
			fsblock *f = largeroot->clusterBuffer.fs + i;
			if(f->fcode != 0){
				if(flagcheck(f->attr, __fsmetadatacluster) && f->fcode != 0){
					conf_fsblock *temp = readblock(re, getloc(largeroot, f), 1);
					if(__memcmp(temp->fsig, FRATBLOCKSIG, 8)){
						Print(L"\nERROR!\nCorrupted FileSystem Root Block\nERASING ENTRY!!");
						__memset(temp, 0, 512);
						f->fcode = 0;
						writeblock(re, temp, getloc(largeroot, f), 1);
					}
					FreePool(temp);
				}
			}
		}
		dispose(re);
		return largeroot;
	}
}

fsblock *allocatecluster(conf_fsroot *root){
	for(size_t i = root->lastClusterAlloc; i < root->clusterBuffer.clusterSize; ++i){
		if(root->clusterBuffer.fs[i].fcode == 0){
			root->lastClusterAlloc = i;
			return root->clusterBuffer.fs + i;
		}
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
	for(size_t cc = 0; cc < root->clusterBuffer.clusterSize; ++cc){
		if(root->clusterBuffer.fs[cc].fcode == hash){root->clusterBuffer.fs[cc].fcode = 0;}
	}
}

void __fcreate(conf_fsroot *root, char *path, char *flags){
#ifdef __DEBUG__
	Print(L"\nCreating File at: ./%a", path);
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
		Print(L"\n\tFcode: %llu", fb->fcode);
#endif
		fb->index = 0;
		dirhandle *dhandle = __fgetparent(root, path);
		if(dhandle && dhandle->file->fcode != fb->fcode){
			__fdiradd(dhandle, fb);
			__fuloaddir(dhandle);
		}
		char *_path = AllocatePool(__strlen(path) + 2);
		*_path = '\\';
		__memcpy(_path + 1, path, __strlen(path) + 1);
		__finit(root, fb, _path);
	}
}

fsblock *__faddr(conf_fsroot *root, fsblock *family){
#ifdef __DEBUG__
	Print(L"\nAdding FS Table Entry: %llu, Type: %a", family->fcode, (flagcheck(family->attr, __fsdirectory)? "DIRECTORY": "FILE"));
#endif
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 0;
		for(size_t i = 0; i < root->clusterBuffer.clusterSize; ++i){
			if(root->clusterBuffer.fs[i].fcode == family->fcode){fb->index++;}
		}
		fb->attr = family->attr;
		fb->fcode = family->fcode;
		flagunset(fb->attr, __fsmetadatacluster);
		// Clear Block
		void *bl0 = __calloc(1, root->root->confBlockSize);
		__memset(bl0, 0, root->root->confBlockSize);
		LBA loc = getloc(root, fb);
#ifdef __DEBUG__
	Print(L"\n\tWriting 0-Block: %llu", loc);
#endif
		rawenv re = startup(root->MediaID, root->root->confBlockSize);
		writeblock(re, bl0, loc, 1);
		FreePool(bl0);
		dispose(re);
	}
	return fb;
}

void __finit(conf_fsroot *root, fsblock *fb, char *path){
#ifdef __DEBUG__
	Print(L"\nInitialising File MetaData: ./%a", path);
#endif
	LBA loc = getloc(root, fb);
	void *block = AllocatePool(root->root->confBlockSize);
	conf_fsblock *metadata = (conf_fsblock *)block;
	EFI_TIME time;
	if(EFI_ERROR(gRT->GetTime(&time, NULL))){time = (EFI_TIME){0};}
	char *name;
	for(INT64 i = __strlen(path) - 1; i > -1; i--){
		if(i > GPTeNAMELEN){path[i] = '\0';}else{
			if(path[i] == '/' || path[i] == '\\'){
				name = path + __strlen(path) - i;  break;
			}else if(!isascii(path[i])){path[i] = 0;}
		}
	}
	*metadata = (conf_fsblock){
		.accessdate = time.Year,
		.writedate = 0,
		.accesstime = (time.Hour * 3600) + (time.Minute * 60) + time.Second,
		.writetime = 0,
		.attributes = fb->attr,
		.fcode = (flagcheck(fb->attr, __fsproxy)? 0: fb->fcode),
	};
	__memcpy(metadata->fsig, FRATBLOCKSIG, 8);
	flagunset(metadata->attributes, __fsmetadatacluster);
	__memset(metadata->name, 0, GPTeNAMELEN);
	__memcpy(metadata->name, name, __strlen(name));
	rawenv re = startup(root->MediaID, root->root->confBlockSize);
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
		loc = getloc(root, fb_);
	}else{loc = getloc(root, fb);}
#ifdef __DEBUG__
	Print(L"\nReading File Block at %llu, Item: %llu.\tRoot: %llu", loc, i, fb->fcode);
#endif
	rawenv re = startup(root->MediaID, root->root->confBlockSize);
	void *out = readblock(re, loc, 1);
	dispose(re);
	return out;
}

fsblock *__ffindh(conf_fsroot *root, size_t hash){
	hash &= 0x3FFFFFFFFFF;
	for(size_t cc = 0; cc < root->clusterBuffer.clusterSize; ++cc){
		if(root->clusterBuffer.fs[cc].fcode == hash && root->clusterBuffer.fs[cc].index == 0){
			return root->clusterBuffer.fs + cc;
		}
	}
	return NULL;
}

fsblock *__ffindhi(conf_fsroot *root, size_t hash, size_t index){
	hash &= 0x3FFFFFFFFFF;
	for(size_t cc = 0; cc < root->clusterBuffer.clusterSize; ++cc){
		if(root->clusterBuffer.fs[cc].fcode == hash && root->clusterBuffer.fs[cc].index == index){
			return root->clusterBuffer.fs + cc;
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
			Print(L"\nError: Attempted Write to Protected Metadata Cluster");
#endif
			return;
		}else{loc = getloc(root, fb);}
		
	}
#ifdef __DEBUG__
	Print(L"\nWriting File Block at %llu, Item: %llu.\tRoot: %llu", loc, i, fb->fcode);
#endif
	rawenv re = startup(root->MediaID, root->root->confBlockSize);
	writeblock(re, buffer, loc, 1);
	dispose(re);
}

dirhandle *__floaddir(conf_fsroot *root, char *path, char *args){
#ifdef __DEBUG__
	Print(L"\nMounting Dir [%a] with Args: \"%a\"", path, args);
#endif
	dirhandle *out = AllocatePool(sizeof(dirhandle));
	out->root = root;
	out->path = __strdup(path);
	out->file = __ffind(root, path);
	out->dirarray = NULL;
	if(!out->file && strcheck(args, 'c')){
		__fcreate(root, path, args);
		out->file = __ffind(root, path);
		if(!out->file){return NULL;}
	}
	if(flagcheck(out->file->attr, __fsdirectory)){__fdirrefresh(out);
	}else{
		FreePool(out->path);
		FreePool(out);
		return NULL;
	}
	return out;
}

void __fuloaddir(dirhandle *handle){
#ifdef __DEBUG__
	Print(L"\nUn-Mounting Directory [%a]", handle->path);
#endif
	// Flush root
	rawenv re = startup(handle->root->MediaID, handle->root->root->confBlockSize);
	writeblock(re, handle->root->root, handle->root->loc, 1);
	// Flush cluster Array/Buffer
	writeblock(re, handle->root->clusterBuffer.fs, handle->root->clusterBuffer.clusterLBA, handle->root->clusterBuffer.clusterSectors);	
	// Flush Entries
	size_t entry = 1;
	fsblock *f = NULL;
	do{
		f = __ffindhi(handle->root, handle->file->fcode, entry);
		if(f){writeblock(re, handle->dirarray + (getblocksize(re) * (entry - 1)), handle->root->loc + 2 + handle->root->clusterBuffer.clusterSectors + (((size_t)f - (size_t)handle->root->clusterBuffer.fs) / sizeof(fsblock)), 1);}
		entry++;
	}while(f);
	dispose(re);
	FreePool(handle->dirarray);
	FreePool(handle->path);
	FreePool(handle);
}

void __fdirrefresh(dirhandle *handle){
#ifdef __DEBUG__
	Print(L"\nRefreshing Dir %a", handle->path);
#endif
	rawenv re = startup(handle->root->MediaID, handle->root->root->confBlockSize);
	size_t blockprogress = 0;
	BOOLEAN exit_ = FALSE;
	do{
		// Find the associated Clusters/Blocks
		fsblock *fb = __ffindi(handle->root, handle->path, blockprogress + 1);
		if(fb){
			// Read off the Block 
			handle->dirarray = __realloc(handle->dirarray, handle->loadedblocks * getblocksize(re), getblocksize(re) * (blockprogress + 1));
			void *temp = readblock(re, getloc(handle->root, fb), 1);
			__memcpy(handle->dirarray + (getblocksize(re) * blockprogress), temp, getblocksize(re));
			FreePool(temp);
			for(size_t cc = 0; cc < getblocksize(re) / sizeof(diritem); ++cc){if(handle->dirarray[cc].local == 0){exit_ = TRUE;	break;}}
			blockprogress++;
		}else if(!fb){exit_ = TRUE;}
	}while(!exit_);
	handle->loadedblocks = blockprogress;
	dispose(re);
}

fhandle *fsloadh(conf_fsroot *root, char *path, char *args){
#ifdef __DEBUG__
	Print(L"\nMounting File [%a] with \"%a\"", path, args);
#endif
	// size_t proxyhash = 0;
	if(strcheck(args, 'c')){__fcreate(root, path, args);}
	// if(strcheck(args, 'P')){
	// 	conf_fsblock *finfo = __freadinfo(root, __ffind(root, path));
	// 	if(!strcheck(args, 'R')){
	// 		if(finfo->fcode){return fsloadh(root, path, args);}
	// 	}
	// }
	fhandle *out = AllocatePool(sizeof(fhandle));
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
	Print(L"\nUn-Mounting File [%llu]", handle->file->fcode);
#endif
	// Flush root
	rawenv re = startup(handle->root->MediaID, handle->root->root->confBlockSize);
	writeblock(re, handle->root->root, handle->root->loc, 1);
	// Flush cluster Array/Buffer
	writeblock(re, handle->root->clusterBuffer.fs, handle->root->clusterBuffer.clusterLBA, handle->root->clusterBuffer.clusterSectors);
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
			FreePool(handle->handlecache[cc].block);
		}
	}
	FreePool(handle);
	dispose(re);
}


conf_fsblock *__freadinfo(conf_fsroot *root, fsblock *fb){
#ifdef __DEBUG__
	Print(L"\nReading File Info.\tRoot: %llu", fb->fcode);
#endif
	LBA loc = getloc(root, fb);
	rawenv re = startup(root->MediaID, root->root->confBlockSize);
	void *out = readblock(re, loc, 1);
	dispose(re);
	return out;
}

void __fupdatetstamp(conf_fsroot *root, fsblock *file, BOOLEAN wt){
	rawenv re = startup(root->MediaID, root->root->confBlockSize);
	conf_fsblock *finfo = __freadinfo(root, file);
	LBA loc = getloc(root, file);
	EFI_TIME time;
	if(!EFI_ERROR(gST->RuntimeServices->GetTime(&time, NULL))){
		if(wt){
			finfo->writetime = (time.Hour * 3600) + (time.Minute * 60) + time.Second;
			finfo->writedate = time.Day;
		}
		finfo->accesstime = (time.Hour * 3600) + (time.Minute * 60) + time.Second;
		finfo->accessdate = time.Day;
		writeblock(re, finfo, loc, 1);
	}
	dispose(re);
}

conf_fsblock *_dreadinfo(dirhandle *handle){return __freadinfo(handle->root, handle->file);}
conf_fsblock *_freadinfo(fhandle *handle){return __freadinfo(handle->root, handle->file);}

void _fseek(fhandle *handle, size_t progress){handle->progress = progress;}
void _fseeko(fhandle *handle, INT64 progress){handle->progress += progress;}

void _fpush1(fhandle *handle, void *buffer){
	// Update TimeStamp
	__fupdatetstamp(handle->root, handle->file, TRUE);
	if(!flagcheck((__ffindh(handle->root, handle->file->fcode))->attr, __fsreadonly)){
		for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
			if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit || handle->handlecache[cc].progresstimestamp == -1){
				if(handle->handlecache[cc].block){FreePool(handle->handlecache[cc].block);}
				handle->handlecache[cc].block = __memdup(buffer, handle->root->root->confBlockSize);
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
#ifdef __DEBUG__
	Print(L"\nReading 1 Block from Block: %llu", getloc(handle->root, handle->file));
#endif
	// Update TimeStamp
	__fupdatetstamp(handle->root, handle->file, TRUE);
	for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
		if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit || handle->handlecache[cc].progresstimestamp == -1){
			if(handle->handlecache[cc].block){FreePool(handle->handlecache[cc].block);}
			void *buffer = __fread1(handle->root, handle->file, handle->progress);
			handle->handlecache[cc].block = __memdup(buffer, handle->root->root->confBlockSize);
			handle->handlecache[cc].progresstimestamp = handle->progress;
			handle->handlecache[cc].rw = 0;
			handle->progress++;
			return buffer;
		}
	}
	return NULL;
}

dirhandle *__fgetparent(conf_fsroot *root, char *path){
	uint32_t cc = __strlen(path);
	char *dup = __strdup(path);
	while(dup[cc] == PATHnoSEP || dup[cc] == PATHSEP){cc--;}
	for(; cc > 0; --cc){if(dup[cc] == PATHSEP || dup[cc] == PATHnoSEP){dup[cc] = '\0';	break;}}
	if(cc == 0){return NULL;}
	dirhandle *out = __floaddir(root, dup, "dc");
	FreePool(dup);
	return out;
}

void __fdiradd(dirhandle *dir, fsblock *fb){
#ifdef __DEBUG__
	Print(L"\nAdding Entry:%llu to Dir [%a:%llu]", fb->fcode, dir->path, dir->file->fcode);
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
	}while(TRUE);
}

BOOLEAN __ftest(fhandle *h){
#ifdef __DEBUG__
	Print(L"\nPerforming File Test. Code: %llu", h->file->fcode);
#endif
	BOOLEAN out = TRUE;
	void *_block = AllocatePool(h->root->root->confBlockSize);
	trng__(_block, h->root->root->confBlockSize);
	_fseek(h, 1);
	_fpush1(h, _block);
	_fseeko(h, -1);
	void *__block = _fread1(h);
	if(__block){
		Print(L"\n");
		for(size_t cc = 0; cc < h->root->root->confBlockSize; ++cc){
			if(((uint8_t *)_block)[cc] != ((uint8_t *)__block)[cc]){
				out = FALSE;
#ifdef __DEBUG__
				Print(L"✖ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
			}else{
				Print(L"✔ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
#endif
			}
		}
	   FreePool(__block);
	}
	FreePool(_block);
	return out;
}

unhandle *__fdirlist(dirhandle *dir, size_t *index){
	__fdirrefresh(dir);
	if((((*index) * sizeof(diritem)) / dir->root->root->confBlockSize) < dir->loadedblocks){
		diritem *ditem = dir->dirarray + (*index);
		(*index)++;
		fsblock *fb = __ffindh(dir->root, ditem->fcode);
		if(fb){
			if(fb->fcode == ditem->fcode && (fb->fcode != 0)){
				conf_fsblock *finfo = __freadinfo(dir->root, fb);
				char *temp = __strdup(dir->path);
				temp = __realloc(temp, __strlen(dir->path), __strlen(temp) + __strlen(finfo->name) + 2);
				__memcpy(temp + __strlen(dir->path) + (*finfo->name != '/'), finfo->name, __strlen(finfo->name) + 1);
				temp[__strlen(dir->path)] = PATHSEP;
				FreePool(finfo);
				unhandle *out = AllocatePool(sizeof(unhandle));
				if((out->dir = flagcheck(fb->attr, __fsdirectory))){
					out->dhandle_ = __floaddir(dir->root, temp, "");
				}else{out->fhandle_ = fsloadh(dir->root, temp, "f");}
				FreePool(temp);
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
	dirrunner *dr = __calloc(1, sizeof(dirrunner));
	if(!dr){return NULL;}
	dr->dir = handle;
	dr->pattmatcher = patternmatcher;
	dr->stack_size = 8;
	dr->stack_depth = 1;
	dr->dir_stack = __calloc(dr->stack_size, sizeof(dirhandle*));
	dr->idx_stack = __calloc(dr->stack_size, sizeof(size_t));
	if(!dr->dir_stack || !dr->idx_stack){
		FreePool(dr->dir_stack);
		FreePool(dr->idx_stack);
		FreePool(dr);
		return NULL;
	}
	dr->dir_stack[0] = handle;
	dr->idx_stack[0] = 0;
	return dr;
}

static BOOLEAN __dirr_matches(dirrunner *dr, unhandle *item){
	conf_fsblock *info = item->dir ? _dreadinfo(item->dhandle_) : _freadinfo(item->fhandle_);
	if(!info){return FALSE;}
	BOOLEAN match = __pattmatch(dr->pattmatcher, info->name);
	FreePool(info);
	return match;
}

static BOOLEAN __dirr_push(dirrunner *dr, dirhandle *handle){
	if(dr->stack_depth == dr->stack_size){
		size_t new_size = dr->stack_size ? dr->stack_size * 2 : 8;
		dr->dir_stack = __realloc(dr->dir_stack, dr->stack_size * sizeof(dirhandle *), new_size * sizeof(dirhandle*));
		dr->idx_stack = __realloc(dr->idx_stack, dr->stack_size * sizeof(dirhandle *), new_size * sizeof(size_t));
		if(!dr->dir_stack || !dr->idx_stack) return FALSE;
		dr->stack_size = new_size;
	}
	dr->dir_stack[dr->stack_depth] = handle;
	dr->idx_stack[dr->stack_depth] = 0;
	dr->stack_depth++;
	return TRUE;
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
	FreePool(dr->dir_stack);
	FreePool(dr->idx_stack);
	FreePool(dr);
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

		BOOLEAN matches = __dirr_matches(dr, item);
		if(item->dir){
			if(!__dirr_push(dr, item->dhandle_)){
				FreePool(item);
				return NULL;
			}
		}
		if(matches){return item;}
		FreePool(item);
	}

	return NULL;
}

void __fprint_info(conf_fsblock *finfo){
	Print(
		L"\nFile Info:"
		L"\nSignature: %.8s"
		L"\nVersion: %llu"
		L"\nfCode: %llu"
		L"\nAttributes:"
		L"\n\tIs Directory: [%a]"
		L"\n\tIs File: [%a]"
		L"\n\tIs Readonly: [%a]"
		L"\nName: %.32s"
		L"\nAccess Time: %u:%u:%u"
		L"\nWrite Time: %u:%u:%u"
		L"\nAccess Date: %u"
		L"\nWrite Date: %u",
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

LBA getloc(conf_fsroot *root, fsblock *fb){
	return root->clusterBuffer.clusterLBA + root->clusterBuffer.clusterSectors + (((size_t)fb - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
}