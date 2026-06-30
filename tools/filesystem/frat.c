#include "frat.h"

bool checkdisk(char *path){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nVerifying Disk GPT");
#endif
	bool out = false;
	rawenv re = startup(path, __FS_DEFAULTBLOCKSIZE);
	uint8_t *block = readblocks(re, GPT_LBA, sizeof(miniGPT));
	miniGPT *gpt = (miniGPT *)block;
#ifdef __DEBUG__
	dualprintf(logf, stdout, 
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
	dualprintf(logf, stdout, 
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
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nGPT Exists? %s", ((out == true)? "true": "false"));
#endif
	return out;
}

partdim loadpart(char *path, GPTeNSTR name){
#ifdef __DEBUG__
	char gptname[GPTeNAMELEN + 1];	memset(gptname, GPTeNAMESIZE + 1, 0);
	for(uint8_t cc = 0; cc < GPTeNAMELEN; ++cc){gptname[cc] = (name[cc] & 0xFF);}
	dualprintf(logf, stdout, "\n[%s:%u]  >>  Loading Partition:    %s", __FILE__, __LINE__, gptname);
#endif
	if(checkdisk(path)){
		rawenv re = startup(path, __FS_DEFAULTBLOCKSIZE);
		miniGPT *gpt = (miniGPT *)readblocks(re, GPT_LBA, sizeof(miniGPT));
		GPTentry *ge = (GPTentry *)readblocks(re, gpt->partEntryLoc, gpt->nPartEntries * sizeof(GPTentry));
		dispose(re);
		for(uint32_t i = 0; i < gpt->nPartEntries; i++){
			if(!memcmp(name, ge[i].name, GPTeNAMESIZE)){
				partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA, .e = ge[i]};
				free(gpt);
				free(ge);
#ifdef __DEBUG__
				dualprintf(logf, stdout, "\n[%s:%u]  >>  Found Partition: %llu:%llu", __FILE__, __LINE__, out.base, out.high);
#endif
				return out;
			}
		}
		free(gpt);
		free(ge);
	}
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\n[%s:%u]  >>  Found No Partition named: %s", __FILE__, __LINE__, gptname);
#endif
	return (partdim){0, 0, {0}};
}

void formatpart(
	char *path,
	GPTeNSTR name, 
	uint32_t confBlockSize, uint32_t confLogSectors, 
	uint32_t verMAJOR, uint32_t verMINOR
){
#ifdef __DEBUG__
	char gptname[GPTeNAMELEN + 1];	memset(gptname, GPTeNAMESIZE + 1, 0);
	for(uint8_t cc = 0; cc < GPTeNAMELEN; ++cc){gptname[cc] = (name[cc] & 0xFF);}
	dualprintf(logf, stdout, "\n[%s:%u]  >>  Formatting Partition:    %s", __FILE__, __LINE__, gptname);
#endif
	partdim part = loadpart(path, name);
	if(!part.high){return;}
	rawenv re = startup(path, confBlockSize);
#ifdef __DEBUG__
	dualprintf(logf, stdout, 
		"\n[%s:%u]  >>  Format Target: %llu:%llu -> %llu LBAs"
		"\n    Version-Code: [%u:%u]"
		"\n    Configured Block-Size: %u"
		"\n    Configured Cluster-Size: %u"
		"\n    Configured # of Log-Sectors: %u"
		"\n    Configured Log-Size: %u",
		__FILE__, __LINE__, 
		part.base, part.high, part.high - part.base, 
		verMAJOR, verMINOR, confBlockSize, 
		CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize), 
		confLogSectors, confLogSectors * sizeof(fslogitem)
	);
#endif

	// Root-Block
	void *block = calloc(confLogSectors, confBlockSize);
	// ZeroMem(block, confBlockSize * confLogSectors);
	fsroot *fr = block;
	*fr = (fsroot){
		.confLogSectors = confLogSectors,
		.confBlockSize = confBlockSize,
		.confClusterSize = CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize),
		.verCode = MAKEVERSION(verMAJOR, verMINOR),
	};
	// Write FSROOT
	memcpy(fr->signature, FRATSIG, sizeof(FRATSIG));
	writeblocks(re, block, part.base + FRATROOTOFFSET, sizeof(fsroot));

	// Log-Block
	memset(block, 0, confBlockSize * confLogSectors);
	writeblocks(re, block, part.base + LOGBLOCKOFFSET, confBlockSize * confLogSectors);

	// Cluster-Map
	free(block);
	block = calloc(CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize), confBlockSize);
	// memset(block, 0, CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize);
	writeblocks(re, block, part.base + CLUSTERMAPOFFSET(confLogSectors), CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize);
	free(block);
	dispose(re);
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\n[%s:%u]  >>  Formatted Partition", __FILE__, __LINE__);
#endif
}

partdim queryparttablefs(miniGPT *gpt, rawenv re){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\n[%s:%u]  >>  Querying Part Table", __FILE__, __LINE__);
#endif
	// Query all Partitions for the FileSystem.
	GPTentry *ge = (GPTentry *)readblocks(re, gpt->partEntryLoc, gpt->nPartEntries * sizeof(GPTentry));
	for(uint32_t i = 0; i < gpt->nPartEntries; i++){
		char gptname[GPTeNAMELEN + 1];	memset(gptname, GPTeNAMESIZE + 1, 0);
		for(uint8_t cc = 0; cc < GPTeNAMELEN; ++cc){gptname[cc] = (ge[i].name[cc] & 0xFF);}
#ifdef __DEBUG__
		dualprintf(logf, stdout, "\n[%s:%u]  >>  GPT-Entry: %s{%llu:%llu -> %llu}", __FILE__, __LINE__, gptname, ge[i].sLBA, ge[i].eLBA, ge[i].eLBA - ge[i].sLBA);
#endif
		if(queryfs(re, ge[i].sLBA)){
			partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA, .e = ge[i]};
			free(ge);
			return out;
		}
	}
	free(ge);
	return (partdim){0, 0, {0}};
}

bool queryfs(rawenv re, LBA partbase){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\n[%s:%u]  >>  Querying FS at %llu", __FILE__, __LINE__, partbase);
#endif
	// Check that a FileSystem exists at the bytebase.
	bool out = 0;
	fsroot *fr = (fsroot *)readblocks(re, partbase + FRATROOTOFFSET, sizeof(fsroot));
	dualprintf(logf, stdout, 
		"\nFS-Root Blob:"
		"\n    Version: [%u:%u]"
		"\n    Sig: %.16s"
		"\n    Configured Log Sectors: %u"
		"\n    Configured Block Size: %u"
		"\n    Configured Cluster-Map Size: %u",
		fr->verCode[0], fr->verCode[1], 
		fr->signature, fr->confLogSectors, 
		fr->confBlockSize, fr->confClusterSize
	);
	if(!memcmp(fr->signature, FRATSIG, sizeof(FRATSIG))){
#ifdef __DEBUG__
		dualprintf(logf, stdout, 
			"\n[%s:%u]  >>  FS Found at [%llu]:"
			"\n	Version-Code: [%u:%u]"
			"\n	Configured Block-Size: %u"
			"\n	Configured Cluster-Size: %u"
			"\n	Configured # of Log-Sectors: %u"
			"\n	Configured Log-Size: %u", 
			__FILE__, __LINE__, partbase, fr->verCode[0], fr->verCode[1], 
			fr->confBlockSize, fr->confClusterSize, fr->confLogSectors, fr->confLogSectors * sizeof(fslogitem)
		);
#endif
		out = true;
	}else{out = false;}
	free(fr);
	return out;
}

conf_fsroot *fmount(char *path){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\n[%s:%u]  >>  Mounting FS Root: [%s]", __FILE__, __LINE__, path);
#endif
	partdim PART;
	if(checkdisk(path)){
		// Get the LBA Info for a Partition
		rawenv re = startup(path, __FS_DEFAULTBLOCKSIZE);
		void *block = readblocks(re, GPT_LBA, sizeof(miniGPT));
		miniGPT *gpt = (miniGPT *)block;
		if(!(PART = queryparttablefs(gpt, re)).high){free(gpt);    return NULL;}
#ifdef __DEBUG__
		dualprintf(logf, stdout, 
			"\n[%s:%u]  >>  Found Formatted Partition: %llu:%llu -> %llu", 
			__FILE__, __LINE__, PART.base, PART.high, PART.high - PART.base
		);
#endif
		// Generate the fsroot
		fsroot *fsroot_ = (fsroot *)readblocks(re, PART.base + FRATROOTOFFSET, sizeof(fsroot));
#ifdef __DEBUG__
		dualprintf(logf, stdout, 
			"\n[%s:%u]  >>  Root: %llu:%llu -> %llu LBAs"
			"\n	Version-Code: [%u:%u]"
			"\n	Configured Block-Size: %u"
			"\n	Configured Cluster-Size: %u"
			"\n	Configured # of Log-Sectors: %u"
			"\n	Configured Log-Size: %u", 
			__FILE__,__LINE__, PART.base, PART.high, PART.high - PART.base, 
			fsroot_->verCode[0], fsroot_->verCode[1], 
			fsroot_->confBlockSize, CLUSTERMAPSECTORS_CALC(PART.base, PART.high, fsroot_->confLogSectors, fsroot_->confBlockSize), 
			fsroot_->confClusterSize, fsroot_->confLogSectors, fsroot_->confLogSectors * sizeof(fslogitem)
		);
#endif
		setblocksize(re, fsroot_->confBlockSize);
		conf_fsroot *largeroot = calloc(1, sizeof(conf_fsroot));
		*largeroot = (conf_fsroot){
			.loc = 0,
			.root = fsroot_,
			.lastClusterAlloc = 0,
			.logblocks = {
				.logBlock = readblocks(re, PART.base + LOGBLOCKOFFSET, fsroot_->confBlockSize * fsroot_->confLogSectors),
				.nLogSectors = fsroot_->confLogSectors
			},
			.clusterbuffer = {
				.nClusterSectors = __safediv(fsroot_->confClusterSize, fsroot_->confBlockSize),
				.clusterSize = fsroot_->confClusterSize,
				.clusterMap = readblocks(re, PART.base + CLUSTERMAPOFFSET(fsroot_->confLogSectors), fsroot_->confClusterSize)
			},
			.GUID[0] = PART.e.GUID[0],	.GUID[1] = PART.e.GUID[1],
			.altGUID[0] = PART.e.uGUID[0],	.altGUID[1] = PART.e.uGUID[1],
			.path = strdup(path)
		};
#ifdef __DEBUG__
		dualprintf(logf, stdout, "\n    Verifying FS Root Items #items: %llu", (size_t)(fsroot_->confClusterSize / sizeof(fsblock)));
		EnableVerbose(re);
#endif
		for(size_t i = 0; i < (fsroot_->confClusterSize / sizeof(fsblock)); ++i){
			// Read and verify ROOTS
			fsblock *f = largeroot->clusterbuffer.clusterMap + i;
			if(f->fcode != 0){
				if(flagcheck(f->attr, __fsmetadatacluster) && f->fcode != 0){
					meta_fsblock *temp = readblocks(re, getloc(largeroot, f), sizeof(meta_fsblock));
					if(memcmp(temp->fsig, FRATBLOCKSIG, 8)){
#ifdef __DEBUG__
						dualprintf(logf, stdout, 
							"\n[%s:%u]  >>  ERROR!    Corrupted FileSystem Root Block    ERASING ENTRY!!"
							"\n    %llu:%u:%u", 
							__FILE__, __LINE__, f->fcode, f->attr, f->index
						);
#endif
						memset(temp, 0, 512);
						f->fcode = 0;
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
	for(size_t i = root->lastClusterAlloc; i < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++i){
		if(root->clusterbuffer.clusterMap[i].fcode == 0){
			root->lastClusterAlloc = i;
			return root->clusterbuffer.clusterMap + i;
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
	for(size_t cc = 0; cc < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash){root->clusterbuffer.clusterMap[cc].fcode = 0;}
	}
}

void __fcreate(conf_fsroot *root, char *path, char *flags){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nCreating File at: ./%s", path);
#endif
	if(__ffind(root, path)){if(strcheck(flags, 'F')){__ffremove(root, path);}else{return;}}
	fsblock *fb = allocatecluster(root);
	if(fb){
		if(strcheck(flags, 'P')){fb->attr |= __fsproxy;}else{
			if(strcheck(flags, 'd')){fb->attr |= __fsdirectory;}
			if(strcheck(flags, 'f') && !flagcheck(fb->attr, __fsdirectory)){fb->attr |= __fsfile;}
			if(strcheck(flags, 'r')){fb->attr |= __fsreadonly;}}
		fb->attr |= __fsmetadatacluster;
		fb->fcode = __getfcode(path);
#ifdef __DEBUG__
		dualprintf(logf, stdout, "\n\tFcode: %llu", fb->fcode);
#endif
		fb->index = 0;
		dirhandle *dhandle = __fgetparent(root, path);
		if(dhandle && dhandle->file->fcode != fb->fcode){
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
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nAdding FS Table Entry: %llu, Type: %s", family->fcode, (flagcheck(family->attr, __fsdirectory)? "DIRECTORY": "FILE"));
#endif
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 0;
		for(size_t i = 0; i < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++i){
			if(root->clusterbuffer.clusterMap[i].fcode == family->fcode){fb->index++;}
		}
		fb->attr = family->attr;
		fb->fcode = family->fcode;
		flagunset(fb->attr, __fsmetadatacluster);
		// Clear Block
		void *bl0 = calloc(1, root->root->confBlockSize);
		memset(bl0, 0, root->root->confBlockSize);
		LBA loc = getloc(root, fb);
#ifdef __DEBUG__
		dualprintf(logf, stdout, "\n\tWriting 0-Block: %llu", loc);
#endif
		rawenv re = startup(root->path, root->root->confBlockSize);
		writeblocks(re, bl0, loc, root->root->confBlockSize);
		free(bl0);
		dispose(re);
	}
	return fb;
}

void __finit(conf_fsroot *root, fsblock *fb, char *path){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nInitialising File MetaData: ./%s", path);
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
	for(ssize_t i = strlen(path) - 1; i > -1; i--){
		if(i > GPTeNAMELEN){path[i] = '\0';}else{
			if(path[i] == '/' || path[i] == '\\'){
				name = path + strlen(path) - i;  break;
			}else if(!isascii(path[i])){path[i] = 0;}
		}
	}
	*metadata = (meta_fsblock){
		.accessdate = _t->tm_yday,
		.writedate = 0,
		.accesstime = (_t->tm_hour * 3600) + (_t->tm_min * 60) + _t->tm_sec,
		.writetime = 0,
		.attributes = fb->attr,
		.fcode = (flagcheck(fb->attr, __fsproxy)? 0: fb->fcode),
	};
	memcpy(metadata->fsig, FRATBLOCKSIG, 8);
	flagunset(metadata->attributes, __fsmetadatacluster);
	memset(metadata->name, 0, GPTeNAMELEN);
	memcpy(metadata->name, name, strlen(name));
	rawenv re = startup(root->path, root->root->confBlockSize);
	writeblocks(re, metadata, loc, sizeof(meta_fsblock));
	dispose(re);
}

fsblock *__ffindh(conf_fsroot *root, size_t hash){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nRoot is %p", root);
#endif
	hash &= 0x3FFFFFFFFFF;
	for(size_t cc = 0; cc < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash && root->clusterbuffer.clusterMap[cc].index == 0){
			return root->clusterbuffer.clusterMap + cc;
		}
	}
	return NULL;
}

fsblock *__ffindhi(conf_fsroot *root, size_t hash, size_t index){
	hash &= 0x3FFFFFFFFFF;
	for(size_t cc = 0; cc < (root->clusterbuffer.clusterSize / sizeof(fsblock)); ++cc){
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

void *__fread1(conf_fsroot *root, fsblock *fb, size_t index){
	LBA loc = 0;
	if(index != fb->index){
		fsblock *fb_ = __ffindhi(root, fb->fcode, index);
		if(fb_ == NULL){return NULL;}
		loc = getloc(root, fb_);
	}else{loc = getloc(root, fb);}
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nReading File Block at %llu, Item: %llu.    Root: %llu", loc, index, fb->fcode);
#endif
	rawenv re = startup(root->path, root->root->confBlockSize);
	void *out = readblocks(re, loc, root->root->confBlockSize);
	dispose(re);
	return out;
}

void __fpush1(conf_fsroot *root, fsblock *fb, size_t i, void *buffer){
	LBA loc = 0;
	if(i != fb->index){
		fsblock *fb_ = NULL;
		if((fb_ = __ffindhi(root, fb->fcode, i)) == NULL){
			fb_ = __faddr(root, fb);
			if(fb_){loc = getloc(root, fb_);
			}else{return;}
		}else{loc = getloc(root, fb_);}
	}else{
		if(flagcheck(fb->attr, __fsmetadatacluster)){
			// Reject the Write
#ifdef __DEBUG__
			dualprintf(logf, stdout, "\nError: Attempted Write to Protected Metadata Cluster");
#endif
			return;
		}else{loc = getloc(root, fb);}
		
	}
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nWriting File Block at %llu, Item: %llu.\tRoot: %llu", loc, i, fb->fcode);
#endif
	rawenv re = startup(root->path, root->root->confBlockSize);
	writeblocks(re, buffer, loc, root->root->confBlockSize);
	dispose(re);
}

dirhandle *floadhdir(conf_fsroot *root, char *path, char *args){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nMounting Dir [%s] with Args: \"%s\"", path, args);
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
	if(flagcheck(out->file->attr, __fsdirectory)){__fdirrefresh(out);}
	else{
		free(out->path);
		free(out);
		return NULL;
	}
	return out;
}

void fuloaddir(dirhandle *handle){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nUn-Mounting Directory [%s]", handle->path);
#endif
	// Flush root
	rawenv re = startup(handle->root->path, handle->root->root->confBlockSize);
	writeblocks(re, handle->root->root, handle->root->loc, sizeof(fsroot));
	// Flush cluster Array/Buffer
	writeblocks(re, handle->root->clusterbuffer.clusterMap, handle->root->loc + CLUSTERMAPOFFSET(handle->root->logblocks.nLogSectors), handle->root->clusterbuffer.nClusterSectors);	
	// Flush Entries
	size_t entry = 0;
	fsblock *f = NULL;
	do{
		f = __ffindhi(handle->root, handle->file->fcode, entry + 1);
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
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nRefreshing Dir %s", handle->path);
#endif
	rawenv re = startup(handle->root->path, handle->root->root->confBlockSize);
	size_t blockprogress = 0;
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
			for(size_t cc = 0; cc < __safediv(getblocksize(re), sizeof(diritem)); ++cc){if(handle->dirarray[cc].local == 0){exit_ = true;	break;}}
			blockprogress++;
		}else if(!fb){exit_ = true;}
	}while(!exit_);
	handle->loadedblocks = blockprogress;
	dispose(re);
}

fhandle *floadh(conf_fsroot *root, char *path, char *args){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nMounting File [%s] with \"%s\"", path, args);
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
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nUn-Mounting File [%llu]", handle->file->fcode);
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
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nReading File Info");
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

size_t __fsize(fhandle *fh){
	size_t size = 0;
	for(size_t cc = 0; cc < (fh->root->clusterbuffer.clusterSize / sizeof(fsblock)); ++cc){
		size += (
			!flagcheck((fh->root->clusterbuffer.clusterMap + cc)->attr, __fsmetadatacluster) &&
			((fh->root->clusterbuffer.clusterMap + cc)->fcode == fh->file->fcode) 
			? fh->root->root->confBlockSize: 0
		);
	}
	return size;
}
size_t __dsize(dirhandle *dh){
	size_t size = 0;
	for(size_t cc = 0; cc < ((dh->loadedblocks * dh->root->root->confBlockSize) / sizeof(diritem)); ++cc){
		size += (((dh->dirarray + cc)->local != 0) ? dh->root->root->confBlockSize: 0);
	}
	return size;
}

meta_fsblock *_dreadinfo(dirhandle *handle){return __freadinfo(handle->root, handle->file);}
meta_fsblock *_freadinfo(fhandle *handle){return __freadinfo(handle->root, handle->file);}

void _fseeko(fhandle *handle, ssize_t progress){_fseek(handle, handle->progress + progress);}
void _fseek(fhandle *handle, size_t progress){handle->progress = (progress == 0? handle->root->root->confBlockSize: progress);}

size_t _fwrite(fhandle *handle, size_t nbytes, const void *data){
    if(!handle || !data || nbytes == 0){return 0;}
    size_t blkSize = handle->root->root->confBlockSize;
    size_t pos    = handle->progress;      // byte offset in file
    size_t left    = nbytes;
    size_t written = 0;
    __fupdatetstamp(handle->root, handle->file, true);
    if(flagcheck((__ffindh(handle->root, handle->file->fcode))->attr, __fsreadonly)){return 0;}
    while(left > 0){
        size_t blkIndex   = pos / blkSize;
        size_t blkOffset  = (size_t)(pos % blkSize);
        size_t chunk      = blkSize - blkOffset;
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
size_t _fread(fhandle *handle, size_t nbytes, void **dataout){
    if(!handle || nbytes == 0){return 0;}
    size_t blkSize = handle->root->root->confBlockSize;
    size_t pos    = handle->progress;      // byte offset in file
    size_t left    = nbytes;
    size_t written = 0;
	__fupdatetstamp(handle->root, handle->file, false);

    void *out = calloc(1, nbytes);
    if(!out){return 0;}
    while(left > 0){
        size_t blkIndex   = pos / blkSize;
        size_t blkOffset  = (size_t)(pos % blkSize);
        size_t chunk      = blkSize - blkOffset;

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
	for(; cc > 0; --cc){if(dup[cc] == PATHSEP || dup[cc] == PATHnoSEP){dup[cc] = '\0';	break;}}
	if(cc == 0){return NULL;}
	dirhandle *out = floadhdir(root, dup, "dc");
	free(dup);
	return out;
}

void __fdiradd(dirhandle *dir, fsblock *fb){
#ifdef __DEBUG__
	dualprintf(logf, stdout, "\nAdding Entry:%llu to Dir [%s:%llu]", fb->fcode, dir->path, dir->file->fcode);
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
	dualprintf(logf, stdout, "\nPerforming File Test. Code: %llu", h->file->fcode);
#endif
	bool out = true;
	void *_block = calloc(1, h->root->root->confBlockSize);
	trng__(_block, h->root->root->confBlockSize);
	_fseek(h, 0);
	_fwrite(h, h->root->root->confBlockSize, _block);
	_fseek(h, 0);
	void *__block = NULL;
	_fread(h, h->root->root->confBlockSize, &__block);
	if(__block){
		dualprintf(logf, stdout, "\n");
		for(size_t cc = 0; cc < h->root->root->confBlockSize; ++cc){
			if(((uint8_t *)_block)[cc] != ((uint8_t *)__block)[cc]){
				out = false;
#ifdef __DEBUG__
				dualprintf(logf, stdout, "✖ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
			}else{
				dualprintf(logf, stdout, "✔ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
#endif
			}
		}
	   free(__block);
	}
	free(_block);
	return out;
}

unhandle *__fdirlist(dirhandle *dir, size_t *index){
	__fdirrefresh(dir);
	if(__safediv(((*index) * sizeof(diritem)), dir->root->root->confBlockSize) < dir->loadedblocks){
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
				unhandle *out = calloc(1, sizeof(unhandle));
				if((out->dir = flagcheck(fb->attr, __fsdirectory))){
					out->dhandle_ = floadhdir(dir->root, temp, "");
				}else{out->fhandle_ = floadh(dir->root, temp, "f");}
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
		if(dr->dir_stack[cc]){if(cc > 0){fuloaddir(dr->dir_stack[cc]);}}
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
			if(top > 0){fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_){
			if(top > 0){fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_->file){
			if(top > 0){fuloaddir(current);}
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
	dualprintf(logf, stdout, 
		"\nFile Info:"
		"\nSignature: %.8s"
		"\nVersion: %llu"
		"\nfCode: %llu"
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
		(flagcheck(finfo->attributes, __fsdirectory) ? "true": "false"), (flagcheck(finfo->attributes, __fsfile) ? "true": "false"), 
		(flagcheck(finfo->attributes, __fsreadonly) ? "true": "false"), 
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

LBA getloc(conf_fsroot *root, fsblock *fb){
	return DATAFIRST(root) + __safediv(((size_t)fb - (size_t)root->clusterbuffer.clusterMap), sizeof(fsblock));
}