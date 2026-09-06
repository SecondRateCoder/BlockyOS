#include "frat.h"

bool checkdisk(char *path){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nVerifying Disk GPT");
#endif
	bool out = false;
	rawenv re = startup(path, __FS_DEFAULTBLOCKSIZE);
	uint8_t *block = readblocks(re, GPT_LBA, sizeof(miniGPT));
	miniGPT *gpt = (miniGPT *)block;
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, 
		"\nGPT Dump:"
		"\n    Sig: \"%s\""
		"\n    Rev: %u"
		"\n    Header-Size: %u"
		"\n    Header Checksum: %u"
		"\n    localLBA: %llu"
		"\n    altLBA: %llu"
		"\n    firstUsable: %llu"
		"\n    lastUsable: %llu"
		"\n    Disk-GUID: ",
		gpt->sig, gpt->rev, gpt->hSize, gpt->hChecksum,
		gpt->localLBA, gpt->alternateLBA, gpt->fUsable, gpt->lUsable
	);
	dualprintf(fs_logf, stdout, 
		"\n    Partition-Table: %llu"
		"\n    # of Partition-Entries: %u"
		"\n    Partition-Entry Size: %u"
		"\n    Partition-Table Checksum: %u",
		gpt->partEntryLoc, gpt->nPartEntries, gpt->partEntrySize,
		gpt->partArrayChecksum
	);
#endif
	if(!memcmp(gpt->sig, "EFI PART", 8)){out = true;}else{out = false;}
	free(block);
	dispose(re);
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nGPT Exists? %s", ((out == true)? "true": "false"));
#endif
	return out;
}

partdim loadpart(char *path, GPTeNSTR name){
#ifdef _DEBUG
	char gptname[GPTeNAMELEN + 1];	memset(gptname, GPTeNAMESIZE + 1, 0x00);
	for(uint8_t cc = 0x00; cc < GPTeNAMELEN; ++cc){gptname[cc] = (name[cc] & 0xFF);}
	dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Loading Partition:    %s", __FILE__, __LINE__, gptname);
#endif
	if(checkdisk(path)){
		rawenv re = startup(path, __FS_DEFAULTBLOCKSIZE);
		miniGPT *gpt = (miniGPT *)readblocks(re, GPT_LBA, sizeof(miniGPT));
		GPTentry *ge = (GPTentry *)readblocks(re, gpt->partEntryLoc, gpt->nPartEntries * sizeof(GPTentry));
		dispose(re);
		for(uint32_t i = 0x00; i < gpt->nPartEntries; i++){
			if(!memcmp(name, ge[i].name, GPTeNAMESIZE)){
				partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA, .e = ge[i]};
				free(gpt);
				free(ge);
#ifdef _DEBUG
				dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Found Partition: %llu:%llu", __FILE__, __LINE__, out.base, out.high);
#endif
				return out;
			}
		}
		free(gpt);
		free(ge);
	}
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Found No Partition named: %s", __FILE__, __LINE__, gptname);
#endif
	return (partdim){0x00, 0x00, {0x00}};
}

void formatpart(
	char *path,
	GPTeNSTR name, 
	uint32_t confBlockSize, uint32_t confLogSectors, 
	uint32_t verMAJOR, uint32_t verMINOR
){
#ifdef _DEBUG
	char gptname[GPTeNAMELEN + 1];	memset(gptname, GPTeNAMESIZE + 1, 0x00);
	for(uint8_t cc = 0x00; cc < GPTeNAMELEN; ++cc){gptname[cc] = (name[cc] & 0xFF);}
	dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Formatting Partition:    %s", __FILE__, __LINE__, gptname);
#endif
	partdim part = loadpart(path, name);
	if(!part.high){return;}
	rawenv re = startup(path, confBlockSize);
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, 
		"\n[%s:%u]  >>  Format Target: %llu:%llu -> %llu Blocks"
		"\n    Version-Code: [%u:%u]"
		"\n    Configured Block-Size: %u"
		"\n    Configured Cluster-Size: %u"
		"\n    Configured # of Log-Sectors: %u"
		"\n    Configured Log-Size: %u",
		__FILE__, __LINE__, 
		part.base, part.high, part.high - part.base, verMAJOR, verMINOR, confBlockSize, 
		CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize), 
		confLogSectors, confLogSectors * sizeof(fslogitem)
	);
#endif
	// Root-Block
	void *block = calloc(confLogSectors, confBlockSize);
	// ZeroMem(block, confBlockSize * confLogSectors);
	*((fsroot *)block) = (fsroot){
		.confLogSectors = confLogSectors,
		.confBlockSize = confBlockSize,
		.confClusterSize = CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize),
		.verCode = MAKEVERSION(verMAJOR, verMINOR),
	};
	// Write FSROOT
	memcpy(((fsroot *)block)->signature, FRATSIG, sizeof(FRATSIG));
	writeblocks(re, block, part.base + FRATROOTOFFSET, sizeof(fsroot));

	// Log-Block
	memset(block, 0x00, confBlockSize * confLogSectors);
	writeblocks(re, block, part.base + LOGBLOCKOFFSET, confBlockSize * confLogSectors);

	// Cluster-Map
	free(block);
	block = calloc(CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize), confBlockSize);
	// memset(block, 0x00, CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize);
	writeblocks(re, block, part.base + CLUSTERMAPOFFSET(confLogSectors), CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize);
	free(block);
	dispose(re);
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Formatted Partition", __FILE__, __LINE__);
#endif
}

partdim queryparttablefs(miniGPT *gpt, rawenv re){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Querying Part Table", __FILE__, __LINE__);
#endif
	// Query all Partitions for the FileSystem.
	GPTentry *ge = (GPTentry *)readblocks(re, gpt->partEntryLoc, gpt->nPartEntries * sizeof(GPTentry));
	for(uint32_t i = 0x00; i < gpt->nPartEntries; i++){
		char gptname[GPTeNAMELEN + 1];	memset(gptname, 0x00, GPTeNAMESIZE);
		for(uint8_t cc = 0x00; cc < GPTeNAMELEN; ++cc){gptname[cc] = (ge[i].name[cc] & 0xFF);}
#ifdef _DEBUG
		dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  GPT-Entry: %s{%llu:%llu -> %llu}", __FILE__, __LINE__, gptname, ge[i].sLBA, ge[i].eLBA, ge[i].eLBA - ge[i].sLBA);
#endif
		if(queryfs(re, ge[i].sLBA)){
			partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA, .e = ge[i]};
			free(ge);
			return out;
		}
	}
	free(ge);
	return (partdim){0x00, 0x00, {0x00}};
}

bool queryfs(rawenv re, LBA partbase){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Querying FS at %llu", __FILE__, __LINE__, partbase);
#endif
	// Check that a FileSystem exists at the bytebase.
	bool out = 0x00;
	fsroot *fr = (fsroot *)readblocks(re, partbase + FRATROOTOFFSET, sizeof(fsroot));
	dualprintf(fs_logf, stdout, 
		"\nFS-Root Blob:"
		"\n    Version: [%u:%u]"
		"\n    Sig: %.16s"
		"\n    Configured Log Sectors: %u"
		"\n    Configured Block Size: %u"
		"\n    Configured Cluster-Map Size: %u",
		fr->verCode[0x00], fr->verCode[1], 
		fr->signature, fr->confLogSectors, 
		fr->confBlockSize, fr->confClusterSize
	);
	if(!memcmp(fr->signature, FRATSIG, sizeof(FRATSIG))){
#ifdef _DEBUG
		dualprintf(fs_logf, stdout, 
			"\n[%s:%u]  >>  FS Found at [%llu]:"
			"\n	Version-Code: [%u:%u]"
			"\n	Configured Block-Size: %u"
			"\n	Configured Cluster-Size: %u"
			"\n	Configured # of Log-Sectors: %u"
			"\n	Configured Log-Size: %u", 
			__FILE__, __LINE__, partbase, fr->verCode[0x00], fr->verCode[1], 
			fr->confBlockSize, fr->confClusterSize, fr->confLogSectors, fr->confLogSectors * sizeof(fslogitem)
		);
#endif
		out = true;
	}else{out = false;}
	free(fr);
	return out;
}

conf_fsroot *fmount(char *path){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\n[%s:%u]  >>  Mounting FS Root: [%s]", __FILE__, __LINE__, path);
#endif
	partdim PART;
	if(checkdisk(path)){
		// Get the LBA Info for a Partition
		rawenv re = startup(path, __FS_DEFAULTBLOCKSIZE);
		void *block = readblocks(re, GPT_LBA, sizeof(miniGPT));
		miniGPT *gpt = (miniGPT *)block;
		if(!(PART = queryparttablefs(gpt, re)).high){free(gpt);    return NULL;}
#ifdef _DEBUG
		dualprintf(fs_logf, stdout, 
			"\n[%s:%u]  >>  Found Formatted Partition: %llu:%llu -> %llu", 
			__FILE__, __LINE__, PART.base, PART.high, PART.high - PART.base
		);
#endif
		// Generate the fsroot
		fsroot *fsroot_ = (fsroot *)readblocks(re, PART.base + FRATROOTOFFSET, sizeof(fsroot));
#ifdef _DEBUG
		dualprintf(fs_logf, stdout, 
			"\n[%s:%u]  >>  Root: %llu:%llu -> %llu LBAs"
			"\n	Version-Code: [%u:%u]"
			"\n	Configured Block-Size: %u"
			"\n	Configured Cluster-Size: %u"
			"\n	Configured # of Log-Sectors: %u"
			"\n	Configured Log-Size: %u", 
			__FILE__,__LINE__, PART.base, PART.high, PART.high - PART.base, 
			fsroot_->verCode[0x00], fsroot_->verCode[1], 
			fsroot_->confBlockSize, CLUSTERMAPSECTORS_CALC(PART.base, PART.high, fsroot_->confLogSectors, fsroot_->confBlockSize), 
			fsroot_->confClusterSize, fsroot_->confLogSectors, fsroot_->confLogSectors * sizeof(fslogitem)
		);
#endif
		setblocksize(re, fsroot_->confBlockSize);
		conf_fsroot *largeroot = calloc(1, sizeof(conf_fsroot));
		*largeroot = (conf_fsroot){
			.loc = 0x00,
			.root = fsroot_,
			.lastClusterAlloc = 0x00,
			.logblocks = {
				.logBlock = readblocks(re, PART.base + LOGBLOCKOFFSET, fsroot_->confBlockSize * fsroot_->confLogSectors),
				.nLogSectors = fsroot_->confLogSectors
			},
			.clusterbuffer = {
				.nClusterSectors = __safediv(fsroot_->confClusterSize, fsroot_->confBlockSize),
				.clusterSize = fsroot_->confClusterSize,
				.clusterMap = readblocks(re, PART.base + CLUSTERMAPOFFSET(fsroot_->confLogSectors), fsroot_->confClusterSize)
			},
			._GUID[0x00] = PART.e.GUID[0x00],	._GUID[1] = PART.e.GUID[1],
			.altGUID[0x00] = PART.e.uGUID[0x00],	.altGUID[1] = PART.e.uGUID[1],
			.path = strdup(path)
		};
#ifdef _DEBUG
		dualprintf(fs_logf, stdout, "\n    Verifying FS Root Items #items: %llu", (uint64_t)(fsroot_->confClusterSize / sizeof(fsblock)));
		EnableVerbose(re);
#endif
		for(uint64_t i = 0x00; i < (fsroot_->confClusterSize / sizeof(fsblock)); ++i){
			// Read and verify ROOTS
			fsblock *f = largeroot->clusterbuffer.clusterMap + i;
			if(f->fcodehigh && f->fcodelow){
				if(flagcheck(f->attributes, __fsmetadatacluster) && f->fcodehigh && f->fcodelow){
					meta_fsblock *temp = readblocks(re, getloc(largeroot, f), sizeof(meta_fsblock));
					if(memcmp(temp->fsig, FRATBLOCKSIG, 8)){
#ifdef _DEBUG
						dualprintf(fs_logf, stdout, 
							"\n[%s:%u]  >>  ERROR!    Corrupted FileSystem Root Block    ERASING ENTRY!!"
							"\n    [%llu:%llu]:%u:%u", 
							__FILE__, __LINE__, f->fcodelow, f->fcodehigh, f->attributes, f->index
						);
#endif
						memset(temp, 0x00, 512);
						f->fcodehigh = 0x00;	f->fcodelow = 0x00;
						writeblocks(re, temp, getloc(largeroot, f), sizeof(meta_fsblock));
					}
					free(temp);
				}
			}
		}
		dispose(re);
		return largeroot;
	}
	return NULL;
}

fsblock *allocatecluster(conf_fsroot *root){
	for(uint64_t i = root->lastClusterAlloc; i < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++i){
		if(root->clusterbuffer.clusterMap[i].fcodelow == 0x00 && root->clusterbuffer.clusterMap[i].fcodehigh == 0x00){
			root->lastClusterAlloc = i;
			return root->clusterbuffer.clusterMap + i;
		}
	}
	root->lastClusterAlloc = 0x00;
	return NULL;
}

void __ffremove(conf_fsroot *root, char *path){
	fsblock *fb = __ffind(root, path);
	fb->fcodelow = 0x00;	fb->fcodehigh = 0x00;
}
void __ffremoveh(conf_fsroot *root, uint64_t hash[2]){
	hash[1] &= FCODEHASHMASK;
	fsblock *fb = __ffindh(root, hash);
	fb->fcodelow = 0x00;	fb->fcodehigh = 0x00;
}

void __ffremovel(conf_fsroot *root, char *path){return __ffremovelh(root, __getfcode(path));}

void __ffremovelh(conf_fsroot *root, uint64_t hash[2]){
	hash[1] &= FCODEHASHMASK;
	for(uint64_t cc = 0x00; cc < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcodelow == hash[0x00] && 
			root->clusterbuffer.clusterMap[cc].fcodehigh == hash[1]
		){root->clusterbuffer.clusterMap[cc].fcodelow = 0x00;	root->clusterbuffer.clusterMap[cc].fcodehigh = 0x00;}
	}
}

void __fcreate(conf_fsroot *root, char *path, char *flags){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nCreating File at: ./%s", path);
#endif
	if(__ffind(root, path)){if(strcheck(flags, 'F')){__ffremove(root, path);}else{return;}}
	fsblock *fb = allocatecluster(root);
	if(fb){
		if(strcheck(flags, 'P')){fb->attributes |= __fsproxy;}else{
			if(strcheck(flags, 'd')){fb->attributes |= __fsdirectory;}
			if(strcheck(flags, 'f') && !flagcheck(fb->attributes, __fsdirectory)){fb->attributes |= __fsfile;}
			if(strcheck(flags, 'r')){fb->attributes |= __fsreadonly;}}
		fb->attributes |= __fsmetadatacluster;
		uint64_t *hash = __getfcode(path);
		uint64_t fcode[2] = {hash[0], hash[1]};
#ifdef _DEBUG
		dualprintf(fs_logf, stdout, "\n\tFcode: [%llu:%llu]", fcode[0], fcode[1] & FCODEHASHMASK);
#endif
		fb->fcodehigh = (fcode[1] & FCODEHASHMASK);
		fb->fcodelow = fcode[0];
		fb->index = 0x00;
		dirhandle *dhandle = __fgetparent(root, path);
		if(dhandle && (dhandle->file->fcodelow != fb->fcodelow || dhandle->file->fcodehigh != fb->fcodehigh)){
			__fdiradd(dhandle, fb);
			fuloaddir(dhandle);
		}
		char *_path = calloc(1, strlen(path) + 2);
		*_path = '\\';
		memcpy(_path + 1, path, strlen(path) + 1);
		__finit(root, fb, _path);
	}
}

fsblock *__faddr(conf_fsroot *root, fsblock *family){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nAdding FS Table Entry: [%llu:%llu], Type: %s", family->fcodelow, family->fcodehigh, (flagcheck(family->attributes, __fsdirectory)? "DIRECTORY": "FILE"));
#endif
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 0x00;
		for(uint64_t i = 0x00; i < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++i){
			if(root->clusterbuffer.clusterMap[i].fcodelow == family->fcodelow && root->clusterbuffer.clusterMap[i].fcodehigh == family->fcodehigh){fb->index++;}
		}
		fb->attributes = family->attributes;
		fb->fcodelow = family->fcodelow;
		fb->fcodehigh = family->fcodehigh;
		flagunset(fb->attributes, __fsmetadatacluster);
		// Clear Block
		void *bl0 = calloc(1, root->root->confBlockSize);
		memset(bl0, 0x00, root->root->confBlockSize);
		LBA loc = getloc(root, fb);
#ifdef _DEBUG
		dualprintf(fs_logf, stdout, "\n\tWriting 0x00-Block: %llu", loc);
#endif
		rawenv re = startup(root->path, root->root->confBlockSize);
		writeblocks(re, bl0, loc, root->root->confBlockSize);
		free(bl0);
		dispose(re);
	}
	return fb;
}

void __finit(conf_fsroot *root, fsblock *fb, char *path){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nInitialising File MetaData: ./%s", path);
#endif
	LBA loc = getloc(root, fb);
	meta_fsblock *metadata = (meta_fsblock *)calloc(1, root->root->confBlockSize);
	time_t _time;	time(&_time);	struct tm *_t = calloc(1, sizeof(struct tm));
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(_t, &_time);
#else
    localtime_r(_t, &_time);
#endif
	char *name;
	for(uint64_t i = strlen(path) - 1; i > -1; i--){
		if(i > GPTeNAMELEN){path[i] = '\0x00';}else{
			if(path[i] == '/' || path[i] == '\\'){
				name = path + strlen(path) - i;  break;
			}else if(!isascii(path[i])){path[i] = 0x00;}
		}
	}
	*metadata = (meta_fsblock){
		.accessdate = _t->tm_yday,
		.writedate = 0x00,
		.accesstime = (_t->tm_hour * 3600) + (_t->tm_min * 60) + _t->tm_sec,
		.writetime = 0x00,
		.f = {.attributes = fb->attributes, 
			.fcodelow = (flagcheck(fb->attributes, __fsproxy)? 0x00: fb->fcodelow), 
			.fcodehigh = (flagcheck(fb->attributes, __fsproxy)? 0x00: fb->fcodehigh)},
	};
	memcpy(metadata->fsig, FRATBLOCKSIG, 8);
	flagunset(metadata->f.attributes, __fsmetadatacluster);
	memset(metadata->name, 0x00, GPTeNAMELEN);
	memcpy(metadata->name, name, strlen(name));
	rawenv re = startup(root->path, root->root->confBlockSize);
	writeblocks(re, metadata, loc, sizeof(meta_fsblock));
	dispose(re);
}

fsblock *__ffindhi(conf_fsroot *root, uint64_t hash[2], uint64_t index){
	fsblock _temp = {.fcodehigh = hash[1], .fcodelow = hash[0], .index = index, .attributes = 0x00};
	for(uint64_t cc = 0x00; cc < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++cc){
		if((root->clusterbuffer.clusterMap[cc].fcodehigh == _temp.fcodehigh) && 
			(root->clusterbuffer.clusterMap[cc].fcodelow == _temp.fcodelow) && 
			root->clusterbuffer.clusterMap[cc].index == _temp.index
		){return root->clusterbuffer.clusterMap + cc;}
	}
	return NULL;
}

fsblock *__ffindh(conf_fsroot *root, uint64_t *hash){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nRoot is %p", root);
#endif
	return __ffindhi(root, hash, 0x00);
}


fsblock *__ffindi(conf_fsroot *root, char *path, uint64_t index){return __ffindhi(root, __getfcode(path), index);}

fsblock *__ffind(conf_fsroot *root, char *path){
	uint64_t *hash = __getfcode(path);
	return __ffindh(root, hash);
}

void *__fread1(conf_fsroot *root, fsblock *fb, uint64_t index){
	LBA loc = 0x00;
	if(index != fb->index){
		fsblock *fb_ = __ffindhi(root, (uint64_t[2]){fb->fcodelow, fb->fcodehigh}, index);
		if(fb_ == NULL){return NULL;}
		loc = getloc(root, fb_);
	}else{loc = getloc(root, fb);}
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nReading File Block at %llu, Item: %llu.    Root: [%llu:%llu]", loc, index, fb->fcodelow, fb->fcodehigh);
#endif
	rawenv re = startup(root->path, root->root->confBlockSize);
	void *out = readblocks(re, loc, root->root->confBlockSize);
	dispose(re);
	return out;
}

void __fpush1(conf_fsroot *root, fsblock *fb, uint64_t i, void *buffer){
	LBA loc = 0x00;
	if(i != fb->index){
		fsblock *fb_ = NULL;
		if((fb_ = __ffindhi(root, (uint64_t[2]){fb->fcodelow, fb->fcodehigh}, i)) == NULL){
			fb_ = __faddr(root, fb);
			if(fb_){loc = getloc(root, fb_);
			}else{return;}
		}else{loc = getloc(root, fb_);}
	}else{
		if(flagcheck(fb->attributes, __fsmetadatacluster)){
			// Reject the Write
#ifdef _DEBUG
			dualprintf(fs_logf, stdout, "\nError: Attempted Write to Protected Metadata Cluster");
#endif
			return;
		}else{loc = getloc(root, fb);}
		
	}
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nWriting File Block at %llu, Item: %llu.\tRoot: [%llu:%llu]", loc, i, fb->fcodelow, fb->fcodehigh);
#endif
	rawenv re = startup(root->path, root->root->confBlockSize);
	writeblocks(re, buffer, loc, root->root->confBlockSize);
	dispose(re);
}

dirhandle *floadhdir(conf_fsroot *root, char *path, char *args){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nMounting Dir [%s] with Args: \"%s\"", path, args);
#endif
	dirhandle *out = calloc(1, sizeof(dirhandle));
	*out = (dirhandle){
		.root = root,
		.path = strdup(path),
		.file = __ffind(root, path),
		.dirarray = NULL
	};
	if(!out->file && strcheck(args, 'c')){
		__fcreate(root, path, args);
		out->file = __ffind(root, path);
		if(!out->file){return NULL;}
	}
	if(flagcheck(out->file->attributes, __fsdirectory)){__fdirrefresh(out);}
	else{
		free(out->path);
		free(out);
		return NULL;
	}
	return out;
}

void fuloaddir(dirhandle *handle){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nUn-Mounting Directory [%s]", handle->path);
#endif
	// Flush root
	rawenv re = startup(handle->root->path, handle->root->root->confBlockSize);
	writeblocks(re, handle->root->root, handle->root->loc, sizeof(fsroot));
	// Flush cluster Array/Buffer
	writeblocks(re, handle->root->clusterbuffer.clusterMap, handle->root->loc + CLUSTERMAPOFFSET(handle->root->logblocks.nLogSectors), handle->root->clusterbuffer.nClusterSectors);	
	// Flush Entries
	uint64_t entry = 0x00;
	fsblock *f = NULL;
	do{
		f = __ffindhi(handle->root, (uint64_t[2]){handle->file->fcodelow, handle->file->fcodehigh}, entry + 1);
		if(f){writeblocks(
			re, ((void *)handle->dirarray) + (handle->root->root->confBlockSize * entry), 
			getloc(handle->root, f), 
			handle->root->root->confBlockSize);
		}
		entry++;
	}while(f);
	dispose(re);
	free(handle->dirarray);
	free(handle->path);
	free(handle);
}

void fuloadroot(conf_fsroot *fr){
	rawenv re = startup(fr->path, fr->root->confBlockSize);
	writeblocks(re, fr->root, fr->loc, sizeof(fsroot));
	// Flush cluster Array/Buffer
	writeblocks(re, fr->clusterbuffer.clusterMap, fr->loc + CLUSTERMAPOFFSET(fr->logblocks.nLogSectors), fr->clusterbuffer.clusterSize);
	dispose(re);
}

void __fdirrefresh(dirhandle *handle){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nRefreshing Dir %s", handle->path);
#endif
	rawenv re = startup(handle->root->path, handle->root->root->confBlockSize);
	uint64_t blockprogress = 0x00;
	bool exit_ = false;
	do{
		// Find the associated Clusters/Blocks
		fsblock *fb = __ffindi(handle->root, handle->path, blockprogress + 1);
		if(fb){
			// Read off the Block 
			handle->dirarray = realloc(handle->dirarray, getblocksize(re) * (blockprogress + 1));
			void *temp = readblocks(re, getloc(handle->root, fb), handle->root->root->confBlockSize);
			memcpy(handle->dirarray + (getblocksize(re) * blockprogress), temp, getblocksize(re));
			free(temp);
			for(uint64_t cc = 0x00; cc < __safediv(getblocksize(re), sizeof(diritem)); ++cc){if(handle->dirarray[cc].local == 0x00){exit_ = true;	break;}}
			blockprogress++;
		}else if(!fb){exit_ = true;}
	}while(!exit_);
	handle->loadedblocks = blockprogress;
	dispose(re);
}

fhandle *floadh(conf_fsroot *root, char *path, char *args){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nMounting File [%s] with \"%s\"", path, args);
#endif
	if(strcheck(args, 'c')){__fcreate(root, path, args);}
	fsblock *fb = __ffind(root, path);
	if(fb){
		fhandle *out = calloc(1, sizeof(fhandle));
		*out = (fhandle){
			.file = fb,
			.root = root,
			.path = strdup(path),
			.progress = root->root->confBlockSize	// Avoid Metadata Block
		};
		return out;
	}
	return NULL;
}

void fuloadh(fhandle *handle){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nUn-Mounting File [%llu:%llu]", handle->file->fcodelow, handle->file->fcodehigh);
#endif
	// Flush root
	rawenv re = startup(handle->root->path, handle->root->root->confBlockSize);
	writeblocks(re, handle->root->root, handle->root->loc, sizeof(fsroot));
	// Flush cluster Array/Buffer
	writeblocks(re, handle->root->clusterbuffer.clusterMap, handle->root->loc + CLUSTERMAPOFFSET(handle->root->logblocks.nLogSectors), handle->root->clusterbuffer.clusterSize);
	free(handle);
	dispose(re);
}


meta_fsblock *__freadinfo(conf_fsroot *root, fsblock *fb){
	LBA loc = getloc(root, fb);
	rawenv re = startup(root->path, root->root->confBlockSize);
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nReading File Info");
#endif
	void *out = readblocks(re, loc, sizeof(meta_fsblock));
	dispose(re);
	return out;
}

void __fupdatetstamp(conf_fsroot *root, fsblock *file, bool wt){
	rawenv re = startup(root->path, root->root->confBlockSize);
	meta_fsblock *finfo = __freadinfo(root, file);
	LBA loc = getloc(root, file);
	time_t _time;	time(&_time);
	struct tm *_t = calloc(1, sizeof(struct tm));
#if defined(_WIN32) || defined(_WIN64)
    // Windows safe version (arguments are inverted)
    localtime_s(_t, &_time);
#else
    // POSIX (Linux/macOS) safe version
    localtime_r(_t, &_time);
#endif
	if(wt){
		finfo->writetime = (_t->tm_hour * 3600) + (_t->tm_min * 60) + _t->tm_sec;
		finfo->writedate = _t->tm_yday;
	}
	finfo->accesstime = (_t->tm_hour * 3600) + (_t->tm_min * 60) + _t->tm_sec;
	finfo->accessdate = _t->tm_yday;
	writeblocks(re, finfo, loc, sizeof(meta_fsblock));
	dispose(re);
}

uint64_t __fsize(fhandle *fh){
	uint64_t size = 0x00;
	for(uint64_t cc = 0x00; cc < (fh->root->clusterbuffer.clusterSize / sizeof(fsblock)); ++cc){
		size += (
			!flagcheck((fh->root->clusterbuffer.clusterMap + cc)->attributes, __fsmetadatacluster) &&
				((fh->root->clusterbuffer.clusterMap + cc)->fcodelow == fh->file->fcodelow && 
					(fh->root->clusterbuffer.clusterMap + cc)->fcodehigh == fh->file->fcodehigh) 
			? fh->root->root->confBlockSize: 0x00
		);
	}
	return size;
}
uint64_t __dsize(dirhandle *dh){
	uint64_t size = 0x00;
	for(uint64_t cc = 0x00; cc < ((dh->loadedblocks * dh->root->root->confBlockSize) / sizeof(diritem)); ++cc){
		size += (((dh->dirarray + cc)->local) ? dh->root->root->confBlockSize: 0x00);
	}
	return size;
}

meta_fsblock *_dreadinfo(dirhandle *handle){return __freadinfo(handle->root, handle->file);}
meta_fsblock *_freadinfo(fhandle *handle){return __freadinfo(handle->root, handle->file);}

void _fseeko(fhandle *handle, uint64_t progress){_fseek(handle, handle->progress + progress);}
void _fseek(fhandle *handle, uint64_t progress){handle->progress = (progress == 0x00? handle->root->root->confBlockSize: progress);}

uint64_t _fwrite(fhandle *handle, uint64_t nbytes, const void *data){
    if(!handle || !data || nbytes == 0x00){return 0x00;}
    uint64_t blkSize = handle->root->root->confBlockSize, 
			pos = handle->progress,       // byte offset in fil
			left = nbytes, 
			written = 0x00;
    __fupdatetstamp(handle->root, handle->file, true);
    if(flagcheck((__ffindh(handle->root, (uint64_t[2]){handle->file->fcodelow, handle->file->fcodehigh}))->attributes, __fsreadonly)){return 0x00;}
    while(left > 0x00){
        uint64_t blkIndex = pos / blkSize, 
				blkOffset = (uint64_t)(pos % blkSize), 
				chunk = blkSize - blkOffset;
        if(chunk > left){chunk = left;}
        void *blk = __fread1(handle->root, handle->file, blkIndex);
        if(!blk){
            blk = calloc(1, blkSize);
            if(!blk){break;}
        }
        memcpy(blk + blkOffset, data + written, chunk);
        __fpush1(handle->root, handle->file, blkIndex, blk);
        free(blk);
        written      += chunk;
        pos          += chunk;
        left         -= chunk;
    }
    handle->progress = pos;
    return nbytes - left;
}
uint64_t _fread(fhandle *handle, uint64_t nbytes, void **dataout){
    if(!handle || nbytes == 0x00){return 0x00;}
    uint64_t blkSize = handle->root->root->confBlockSize;
    uint64_t pos    = handle->progress;      // byte offset in file
    uint64_t left    = nbytes;
    uint64_t written = 0x00;
	__fupdatetstamp(handle->root, handle->file, false);

    void *out = calloc(1, nbytes);
    if(!out){return 0x00;}
    while(left > 0x00){
        uint64_t blkIndex   = pos / blkSize;
        uint64_t blkOffset  = (uint64_t)(pos % blkSize);
        uint64_t chunk      = blkSize - blkOffset;

        void *blk = __fread1(handle->root, handle->file, blkIndex);
        if(!blk){break;}
        if(chunk > left){chunk = left;}

        memcpy(out + written, blk + blkOffset, chunk);
        free(blk);
        written  += chunk;
        pos += chunk;
        left -= chunk;
    }
    handle->progress = pos;
    *dataout = out;
    return nbytes - left;
}

dirhandle *__fgetparent(conf_fsroot *root, char *path){
	uint32_t cc = strlen(path);
	char *dup = strdup(path);
	while(dup[cc] == PATHnoSEP || dup[cc] == PATHSEP){cc--;}
	for(; cc > 0x00; --cc){if(dup[cc] == PATHSEP || dup[cc] == PATHnoSEP){dup[cc] = '\0';	break;}}
	if(cc == 0x00){return NULL;}
	dirhandle *out = floadhdir(root, dup, "dc");
	free(dup);
	return out;
}

void __fdiradd(dirhandle *dir, fsblock *fb){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nAdding Entry:[%llu:%llu] to Dir [%s:[%llu:%llu]", fb->fcodelow, fb->fcodehigh, dir->path, dir->file->fcodelow, dir->file->fcodehigh);
#endif
	do{
		__fdirrefresh(dir);
		uint32_t cc = 0x00;
		if(dir->dirarray){
			do{
				if(dir->dirarray[cc].f.fcodelow == 0x00 && dir->dirarray[cc].f.fcodehigh == 0x00){
					dir->dirarray[cc].f = *fb;
					dir->dirarray[cc].local = 0x00;
					return;
				}else if(dir->dirarray[cc].f.fcodelow == fb->fcodelow && dir->dirarray[cc].f.fcodehigh == fb->fcodehigh){
					dir->dirarray[cc].f.attributes = fb->attributes;
					return;
				}else{dir->dirarray[cc].local++;}
				cc++;
			}while(dir->dirarray[cc == 0x00? cc: (cc - 1)].local);
		}
		__faddr(dir->root, dir->file);
	}while(true);
}

bool __ftest(fhandle *h){
#ifdef _DEBUG
	dualprintf(fs_logf, stdout, "\nPerforming File Test. Code: [%llu:%llu]", h->file->fcodelow, h->file->fcodehigh);
#endif
	bool out = true;
	void *_block = calloc(1, h->root->root->confBlockSize);
	trng__(_block, h->root->root->confBlockSize);
	_fseek(h, 0x00);
	_fwrite(h, h->root->root->confBlockSize, _block);
	_fseek(h, 0x00);
	void *__block = NULL;
	_fread(h, h->root->root->confBlockSize, &__block);
	if(__block){
		dualprintf(fs_logf, stdout, "\n");
		for(uint64_t cc = 0x00; cc < h->root->root->confBlockSize; ++cc){
			if(((uint8_t *)_block)[cc] != ((uint8_t *)__block)[cc]){
				out = false;
#ifdef _DEBUG
				dualprintf(fs_logf, stdout, "✖ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
			}else{
				dualprintf(fs_logf, stdout, "✔ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
#endif
			}
		}
	   free(__block);
	}
	free(_block);
	return out;
}

unhandle *__fdirlist(dirhandle *dir, uint64_t *index){
	__fdirrefresh(dir);
	if(__safediv(((*index) * sizeof(diritem)), dir->root->root->confBlockSize) < dir->loadedblocks){
		diritem *ditem = dir->dirarray + (*index);
		(*index)++;
		fsblock *fb = __ffindh(dir->root, (uint64_t[2]){dir->file->fcodelow, dir->file->fcodehigh});
		if(fb){
			if(fb->fcodelow == ditem->f.fcodelow && fb->fcodehigh == ditem->f.fcodehigh && (fb->fcodelow && fb->fcodehigh)){
				meta_fsblock *finfo = __freadinfo(dir->root, fb);
				char *temp = strdup(dir->path);
				temp = realloc(temp, strlen(temp) + strlen(finfo->name) + 2);
				memcpy(temp + strlen(dir->path) + (*finfo->name != '/'), finfo->name, strlen(finfo->name) + 1);
				temp[strlen(dir->path)] = PATHSEP;
				free(finfo);
				unhandle *out = calloc(1, sizeof(unhandle));
				if((out->dir = flagcheck(fb->attributes, __fsdirectory))){
					out->dhandle_ = floadhdir(dir->root, temp, "");
				}else{out->fhandle_ = floadh(dir->root, temp, "f");}
				free(temp);
				return out;
			}else{
				// Remove Entry
				ditem->f.fcodehigh = 0x00;
				ditem->f.fcodelow = 0x00;
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
	dr->idx_stack = calloc(dr->stack_size, sizeof(uint64_t));
	if(!dr->dir_stack || !dr->idx_stack){
		free(dr->dir_stack);
		free(dr->idx_stack);
		free(dr);
		return NULL;
	}
	dr->dir_stack[0x00] = handle;
	dr->idx_stack[0x00] = 0x00;
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
		uint64_t new_size = dr->stack_size ? dr->stack_size * 2 : 8;
		dr->dir_stack = realloc(dr->dir_stack, new_size * sizeof(dirhandle*));
		dr->idx_stack = realloc(dr->idx_stack, new_size * sizeof(uint64_t));
		if(!dr->dir_stack || !dr->idx_stack) return false;
		dr->stack_size = new_size;
	}
	dr->dir_stack[dr->stack_depth] = handle;
	dr->idx_stack[dr->stack_depth] = 0x00;
	dr->stack_depth++;
	return true;
}

static dirhandle *__dirr_pop(dirrunner *dr){
	if(dr->stack_depth == 0x00) return NULL;
	dr->stack_depth--;
	dirhandle *out = dr->dir_stack[dr->stack_depth];
	dr->dir_stack[dr->stack_depth] = NULL;
	return out;
}

void __dirr_free(dirrunner *dr){
	if(!dr){return;}
	for(uint64_t cc = 0x00; cc < dr->stack_depth; ++cc){
		if(dr->dir_stack[cc]){if(cc > 0x00){fuloaddir(dr->dir_stack[cc]);}}
	}
	free(dr->dir_stack);
	free(dr->idx_stack);
	free(dr);
}

unhandle *__dirr(dirrunner *dr){
	if(!dr || dr->stack_depth == 0x00){return NULL;}
	while(dr->stack_depth > 0x00){
		uint64_t top = dr->stack_depth - 1;
		dirhandle *current = dr->dir_stack[top];
		uint64_t idx = dr->idx_stack[top];
		unhandle *item = __fdirlist(current, &idx);
		dr->idx_stack[top] = idx;

		if(!item){
			if(top > 0x00){fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_){
			if(top > 0x00){fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_->file){
			if(top > 0x00){fuloaddir(current);}
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
	dualprintf(fs_logf, stdout, 
		"\nFile Info:"
		"\nSignature: %.8s"
		"\nVersion: %llu"
		"\nfCode: [%llu:%llu]"
		"\nAttributes:"
		"\n\tIs Directory: [%s]"
		"\n\tIs File: [%s]"
		"\n\tIs Readonly: [%s]"
		"\nName: %.32s"
		"\nAccess Time: %u:%u:%u"
		"\nWrite Time: %u:%u:%u"
		"\nAccess Date: %u"
		"\nWrite Date: %u",
		finfo->fsig, finfo->headerversion, (uint64_t)finfo->f.fcodelow, finfo->f.fcodehigh,
		(flagcheck(finfo->f.attributes, __fsdirectory) ? "true": "false"), (flagcheck(finfo->f.attributes, __fsfile) ? "true": "false"), 
		(flagcheck(finfo->f.attributes, __fsreadonly) ? "true": "false"), 
		finfo->name, 
		(uint32_t)(__safediv(finfo->accesstime, 3600)), 
		(uint32_t)(__safediv((finfo->accesstime % 3600), 60)),
		(uint32_t)(finfo->accesstime % 60), 
		(uint32_t)(__safediv(finfo->writetime, 3600)), 
		(uint32_t)(__safediv((finfo->writetime % 3600), 60)),
		(uint32_t)(finfo->writetime % 60), 
		finfo->accessdate, finfo->writedate
	);
}

LBA getloc(conf_fsroot *root, fsblock *fb){return DATAFIRST(root) + __safediv(((uint64_t)fb - (uint64_t)root->clusterbuffer.clusterMap), sizeof(fsblock));}