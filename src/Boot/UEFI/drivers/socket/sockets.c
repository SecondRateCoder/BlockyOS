#include "sockets.h"

socket_ret socketfuncprefix __fhandle_sckinfo(struct socket_t *socket, UINT32 Property, UINT32 subProperty){
	unhandle *uh = (unhandle *)socket->persistent;
	switch(Property){
		case 0: {	//	We now need to read the MetaData Block
			//	I genuinely dont care bout freeing this Memory.
			meta_fsblock metadata = {0};	{
				meta_fsblock *_metadata = uh->dir? _dreadinfo(uh->dhandle_): _freadinfo(uh->fhandle_);
				metadata = *_metadata;
				__free(_metadata);
			}
			switch(subProperty){
				case 0: {
					return (socket_ret){
						.data = __memdup(&(metadata.headerversion), sizeof(metadata.headerversion)), 
						.errout = __noerr, .nData = sizeof(metadata.headerversion)
					};
				} case 1: {
					UINT64 temp[2] = {metadata.f.fcodelow, metadata.f.fcodehigh};
					return (socket_ret){
						.data = __memdup(temp, sizeof(temp)), 
						.errout = __noerr, .nData = sizeof(temp)
					};
				} case 2: {
					return (socket_ret){
						.data = __memdup(metadata.fsig, sizeof(metadata.fsig)), 
						.errout = __noerr, .nData = sizeof(metadata.fsig)
					};
				} case 3: {
                    uint64_t temp = strlena(metadata.name);
					return (socket_ret){.data = __memdup(metadata.name, temp), .errout = __noerr, .nData = temp};
				} case 4: {
					return (socket_ret){
						.data = __memdup(&(metadata.accesstime), sizeof(metadata.accesstime)), 
						.errout = __noerr, .nData = sizeof(metadata.accesstime)
					};
				} case 5: {
					return (socket_ret){
						.data = __memdup(&(metadata.writetime), sizeof(metadata.writetime)), 
						.errout = __noerr, .nData = sizeof(metadata.writetime)
					};
				} case 6: {
					return (socket_ret){
						.data = __memdup(&(metadata.accessdate), sizeof(metadata.accessdate)), 
						.errout = __noerr, .nData = sizeof(metadata.accessdate)
					};
				} case 7: {
					return (socket_ret){
						.data = __memdup(&(metadata.writedate), sizeof(metadata.writedate)), 
						.errout = __noerr, .nData = sizeof(metadata.writedate)
					};
				} case 8: {
					if(!uh->dir){
						uint64_t len = __fsize(uh->fhandle_);
						return (socket_ret){.data = __memdup(&len, sizeof(len)), .errout = __noerr, .nData = sizeof(len)};
					}
					return (socket_ret){.data = NULL, .nData = 0x0, .errout = __noimpl_socketfunc};
				} default: {return (socket_ret){.data = NULL, .nData = 0, .errout = __noexist};}
			}
		} case 1: {
			if(uh->dir){
				return (socket_ret){
					.data = __memdup(uh->dhandle_->file, sizeof(uh->dhandle_->file)), 
					.errout = __noerr, .nData = sizeof(uh->dhandle_->file)
				};
			}else{
				return (socket_ret){
					.data = __memdup(uh->fhandle_->file, sizeof(uh->fhandle_->file)), 
					.errout = __noerr, .nData = sizeof(uh->fhandle_->file)
				};
			}
		} case 2: {
			if(uh->dir){
				return (socket_ret){
					.data = __strdup(uh->dhandle_->path), 
					.errout = __noerr, .nData = strlena(uh->dhandle_->path)
				};
			}else{
				return (socket_ret){
					.data = __memdup(&(uh->fhandle_->progress), sizeof(uh->fhandle_->progress)), 
					.errout = __noerr, .nData = sizeof(uh->fhandle_->progress)
				};
			}
		} case 3: {
			if(uh->dir){
				return (socket_ret){
					.data = __memdup(&(uh->dhandle_->loadedblocks), sizeof(uh->dhandle_->loadedblocks)), 
					.errout = __noerr, .nData = sizeof(uh->dhandle_->loadedblocks)
				};
			}else{
				return (socket_ret){
					.data = __strdup(uh->fhandle_->path), 
					.errout = __noerr, .nData = strlena(uh->fhandle_->path)
				};
			}
		} case 4: {
			if(uh->dir){
				return (socket_ret){
					.data = __memdup(&(uh->dhandle_->loadedblocks), sizeof(uh->dhandle_->loadedblocks)), 
					.errout = __noerr, .nData = sizeof(uh->dhandle_->loadedblocks)
				};
			}else{
				return (socket_ret){
					.data = __strdup(uh->fhandle_->path), 
					.errout = __noerr, .nData = strlena(uh->fhandle_->path)
				};
			}
		} case 5: {
			if(uh->dir){
				return (socket_ret){
					.data = __memdup(&(uh->dhandle_->dirarray), sizeof(uh->dhandle_->dirarray)), 
					.errout = __noerr, .nData = sizeof(uh->dhandle_->dirarray)
				};
			}
		} default: {return (socket_ret){.data = NULL, .nData = 0, .errout = __noexist};}
	}
}
socket_ret socketfuncprefix __fhandle_sckwrite(socket_t * socket, void *data, UINT64 posBYTES, UINT64 nBYTES, UINT64 nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Write");
	unhandle *uh = (unhandle *)socket->persistent;
	if(!uh->dir){
		_fseek(uh->fhandle_, posBYTES);
		UINT64 writes = _fwrite(uh->fhandle_, nBYTES, data);
		DEBUGPRINT(L"\nFinished Socket Write");
		return (socket_ret){
			.data = ((writes == nBYTES) ? data: NULL),
			.nData = writes,
			.errout = ((writes == nBYTES)? __noerr: __undeferr)
		};
	}
	DEBUGPRINT(L"\nFailed Socket Write");
	return socketret__noimpl;
}
socket_ret socketfuncprefix __fhandle_sckread(socket_t * socket, UINT64 posBYTES, UINT64 readBYTES, UINT64 nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Read");
	unhandle *uh = (unhandle *)socket->persistent;
	if(!uh->dir){
		_fseek(uh->fhandle_, posBYTES);
		void *temp = NULL;
		const UINT64 rstamp = readBYTES;
		if(rstamp != (readBYTES = _fread(uh->fhandle_, readBYTES, &temp))){__free(temp);	temp = NULL;}
		DEBUGPRINT(L"\nFinished Socket Read");
		return (socket_ret){
			.data = temp,
			.nData = readBYTES,
			.errout = __noerr
		};
	}
	DEBUGPRINT(L"\nFailed Socket Read");
	return socketret__noimpl;
}
socket_ret socketfuncprefix __fhandle_sckclose(socket_t * socket, UINT64 nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Close");
	if(((unhandle *)socket->persistent)->dir){
		fuloaddir(((unhandle *)socket->persistent)->dhandle_);
	}else{fuloadh(((unhandle *)socket->persistent)->fhandle_);}
	__free(socket);
	DEBUGPRINT(L"\nFinished Socket Close");
	return socketret_noerr_empty;
}
socket_ret socketfuncprefix __fhandle_sckOPENchild(socket_t * socket, UINT64 nARGbytes, va_list args){// Handle-Handle
	DEBUGPRINT(L"\nSocket Child Open");
	if(nARGbytes > (sizeof(void *) * 2)){
		char *childPATH = va_arg(args, char *);
		if(((unhandle *)socket->persistent)->dir){
			dirhandle *dh = ((unhandle *)socket->persistent)->dhandle_;
			char *path = __strdup(dh->path);
			char *loadargs = va_arg(args, char *);
			va_end(args);
			path = __realloc(path, __strlen(path), __strlen(path) + __strlen(childPATH) + 1);
			__memcpy(path + __strlen(path), childPATH, __strlen(childPATH) + 1);
			fsblock *fb = __ffind(((unhandle *)socket->persistent)->dhandle_->root, path);
			unhandle *uh = __calloc(1, sizeof(unhandle));
			*uh = (unhandle){
				.dir = flagcheck(fb->attributes, __fsdirectory),
				.__notype = flagcheck(fb->attributes, __fsdirectory)? 
					(void *)floadhdir(((unhandle *)socket->persistent)->dhandle_->root, path, loadargs): 
					(void *)floadh(((unhandle *)socket->persistent)->dhandle_->root, path, loadargs)
			};
			// Open Socket
			socket_t *socket = __calloc(1, sizeof(socket_t));
			*socket = (socket_t){
                .persistent = getptr(uh),
				.close = (socketCLOSE)getptr(__fhandle_sckclose),
				.open = (socketOPENchild)getptr(__fhandle_sckOPENchild),
                .read = (socketREAD)getptr(__fhandle_sckread),
                .write = (socketWRITE)getptr(__fhandle_sckwrite),
				.info = (socketINFO)getptr(__fhandle_sckinfo),
				.raw = {
					.read = (socketREADraw)getptr(__fhandle_sckread),
					.write = (socketWRITEraw)getptr(__fhandle_sckwrite)
				},
			};
			DEBUGPRINT(L"\nFinished Socket Child Open");
			return (socket_ret){
				.errout = __noerr,
				.data = socket,
				.nData = sizeof(socket_t)
			};
		}
	}
	DEBUGPRINT(L"\nFailed Socket Write");
	return socketret__noimpl;
}
socket_ret socketfuncprefix __froot_sckread(socket_t * socket, UINT64 posBYTES, UINT64 readBYTES, UINT64 nARGbytes, ...){return socketret__noimpl;}
socket_ret socketfuncprefix __froot_sckwrite(socket_t * socket, void *data, UINT64 posBYTES, UINT64 nBYTES, UINT64 nARGbytes, ...){return socketret__noimpl;}
socket_ret socketfuncprefix __froot_sckclose(socket_t * socket, UINT64 nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Close");
	conf_fsroot *root = (conf_fsroot *)socket->persistent;
	__free(root->root);
	__free(root->clusterbuffer.clusterMap);
	__free(root);
	__free(socket);
	return (socket_ret){
		.data = NULL, .nData = 0,
		.errout = __noerr
	};
}
socket_ret socketfuncprefix __froot_sckOPENchild(socket_t * socket, UINT64 nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Open");
	if(nARGbytes >= (sizeof(char *) * 2)){
		va_list args;	va_start(args, nARGbytes);
		char *path = va_arg(args, char *);
		char *loadargs = va_arg(args, char *);
		DEBUGPRINT(L"\n\nPath: %p:%a\nLoad-Args: %p:%a", path, path, loadargs, loadargs);
		va_end(args);
		unhandle *uh = __calloc(1, sizeof(unhandle));
		*uh = (unhandle){
			.dir = flagcheck((__ffind((conf_fsroot *)socket->persistent, path))->attributes, __fsdirectory),
			.__notype = flagcheck((__ffind((conf_fsroot *)socket->persistent, path))->attributes, __fsdirectory)? 
				(void *)floadhdir((conf_fsroot *)socket->persistent, path, loadargs):
					(void *)floadh((conf_fsroot *)socket->persistent, path, loadargs)
		};
		// Open Socket
		socket_t *socket = __calloc(1, sizeof(socket_t));
		*socket = (socket_t){
			.open = (socketOPENchild)getptr(__fhandle_sckOPENchild),
			.close = (socketCLOSE)getptr(__fhandle_sckclose),
			.persistent = uh,
			.info = (socketINFO)getptr(__fhandle_sckinfo), 
			.read = (socketREAD)getptr(__fhandle_sckread),
			.write = (socketWRITE)getptr(__fhandle_sckwrite),
			.raw = {
				.read = (socketREAD)getptr(__fhandle_sckread),
				.write = (socketWRITE)getptr(__fhandle_sckwrite)
			}
		};
		return (socket_ret){__noerr, sizeof(socket_t), socket};
	}
	return (socket_ret){__incompatible_arg, 0, NULL};
}
socket_ret socketfuncprefix __froot_sckinfo(struct socket_t *socket, UINT32 Property, UINT32 subProperty){
	conf_fsroot *root = (conf_fsroot *)socket->persistent;
	switch(Property){
		case 0: {
			return (socket_ret){
				.data = __memdup(&(root->GUID), sizeof(root->GUID)), 
				.errout = __noerr, .nData = sizeof(root->GUID)
			};
		} case 1: {
			return (socket_ret){
				.data = __memdup(&(root->altGUID), sizeof(root->altGUID)), 
				.errout = __noerr, .nData = sizeof(root->altGUID)
			};
		} case 2: {
			return (socket_ret){
				.data = __memdup(root->root, sizeof(root->root)), 
				.errout = __noerr, .nData = sizeof(root->root)
			};
		} case 3: {
			return (socket_ret){
				.data = __memdup(&(root->loc), sizeof(root->loc)), 
				.errout = __noerr, .nData = sizeof(root->loc)
			};
		} case 4: {
			return (socket_ret){
				.data = __memdup(&(root->lastClusterAlloc), sizeof(root->lastClusterAlloc)), 
				.errout = __noerr, .nData = sizeof(root->lastClusterAlloc)
			};
		} case 5: {
			switch(subProperty){
				case 0: {
					return (socket_ret){
						.data = __memdup(&(root->logblocks.nLogSectors), sizeof(root->logblocks.nLogSectors)), 
						.errout = __noerr, .nData = sizeof(root->logblocks.nLogSectors)
					};
				} case 1: {
					return (socket_ret){
						.data = __memdup(root->logblocks.logBlock, root->root->confBlockSize * root->logblocks.nLogSectors), 
						.errout = __noerr, .nData = root->root->confBlockSize * root->logblocks.nLogSectors
					};
				} default: {return (socket_ret){.data = NULL, .nData = 0, .errout = __noexist};}
			}
		} case 6: {
			switch(subProperty){
				case 0: {
					return (socket_ret){
						.data = __memdup(&(root->clusterbuffer.clusterSize), sizeof(root->clusterbuffer.clusterSize)), 
						.errout = __noerr, .nData = sizeof(root->clusterbuffer.clusterSize)
					};
				} case 1: {
					return (socket_ret){
						.data = __memdup(&(root->clusterbuffer.nClusterSectors), sizeof(root->clusterbuffer.nClusterSectors)), 
						.errout = __noerr, .nData = sizeof(root->clusterbuffer.nClusterSectors)
					};
				} case 2: {
					return (socket_ret){
						.data = __memdup(&(root->clusterbuffer.nClusterItems), sizeof(root->clusterbuffer.nClusterItems)), 
						.errout = __noerr, .nData = sizeof(root->clusterbuffer.nClusterItems)
					};
				} case 3: {
					return (socket_ret){
						.data = __memdup(root->clusterbuffer.clusterMap, sizeof(fslogitem) * root->clusterbuffer.nClusterItems), 
						.errout = __noerr, .nData = sizeof(fslogitem) * root->clusterbuffer.nClusterItems
					};
				} default: {return (socket_ret){.data = NULL, .nData = 0, .errout = __noexist};}
			}
		} default: {return (socket_ret){.data = NULL, .nData = 0, .errout = __noexist};}
	}
}
socket_ret socketfuncprefix __froot_sckopen(UINT32 ignore, UINT64 nARGbytes, va_list *args){
	DEBUGPRINT(L"\nSocket Open");
	DEBUGPRINT(L"    Opening FS Socket");
	socket_ret sret = {0};
	if(nARGbytes < sizeof(EFI_GUID)){
		sret = (socket_ret){.errout = __incompatible_arg, .data = NULL, .nData = 0};
		return sret;
	}
	EFI_GUID guid = va_arg(*args, EFI_GUID), aGuid = va_arg(*args, EFI_GUID);
	conf_fsroot *root = fmount(guid, aGuid);
	if(root){
		socket_t *socket = __calloc(1, sizeof(socket_t));
		*socket = (socket_t){
			.persistent = (void *)root, 
			.close = (socketCLOSE)getptr(__froot_sckclose), 
			.open = (socketOPENchild)getptr(__froot_sckOPENchild), 
			.read = (socketREAD)getptr(__froot_sckread), 
			.write = (socketWRITE)getptr(__froot_sckwrite), 
			.info = (socketINFO)getptr(__froot_sckinfo), 
			.raw = {
				.read = (socketREADraw)getptr(__froot_sckread),
				.write = (socketWRITEraw)getptr(__froot_sckwrite)
			}
		};
		sret = (socket_ret){
			.errout = __noerr,
			.data = socket,
			.nData = sizeof(socket)
		};
		return sret;
	}
	sret = (socket_ret){__incompatible_arg, 0, NULL};
	return sret;
}