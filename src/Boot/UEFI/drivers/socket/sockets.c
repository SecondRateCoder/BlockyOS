#include "sockets.h"


socket_ret socketfuncprefix __fhandle_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Write");
	unhandle *uh = (unhandle *)socket->persistent;
	if(!uh->dir){
		_fseek(uh->fhandle_, posBYTES);
		UINTN writes = _fwrite(uh->fhandle_, nBYTES, data);
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
socket_ret socketfuncprefix __fhandle_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Read");
	unhandle *uh = (unhandle *)socket->persistent;
	if(!uh->dir){
		_fseek(uh->fhandle_, posBYTES);
		void *temp = NULL;
		const UINTN rstamp = readBYTES;
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
socket_ret socketfuncprefix __fhandle_sckclose(socket_t * socket, UINTN nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Close");
	if(((unhandle *)socket->persistent)->dir){
		fuloaddir(((unhandle *)socket->persistent)->dhandle_);
	}else{fuloadh(((unhandle *)socket->persistent)->fhandle_);}
	__free(socket);
	DEBUGPRINT(L"\nFinished Socket Close");
	return socketret_noerr_empty;
}
socket_ret socketfuncprefix __fhandle_sckOPENchild(socket_t * socket, UINTN nARGbytes, va_list args){// Handle-Handle
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
				.dir = flagcheck(fb->attr, __fsdirectory),
				.__notype = flagcheck(fb->attr, __fsdirectory)? 
					(void *)floadhdir(((unhandle *)socket->persistent)->dhandle_->root, path, loadargs): 
					(void *)floadh(((unhandle *)socket->persistent)->dhandle_->root, path, loadargs)
			};
			// Open Socket
			socket_t *socket = __calloc(1, sizeof(socket_t));
			*socket = (socket_t){
                .persistent = getptr(uh),
				.close = (socketCLOSE *)getptr(__fhandle_sckclose),
				.open = (socketOPENchild *)getptr(__fhandle_sckOPENchild),
                .read = (socketREAD *)getptr(__fhandle_sckread),
                .write = (socketWRITE *)getptr(__fhandle_sckwrite),
				.raw = {
					.read = (socketREADraw *)getptr(__fhandle_sckread),
					.write = (socketWRITEraw *)getptr(__fhandle_sckwrite)
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
socket_ret socketfuncprefix __froot_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...){return socketret__noimpl;}
socket_ret socketfuncprefix __froot_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...){return socketret__noimpl;}
socket_ret socketfuncprefix __froot_sckclose(socket_t * socket, UINTN nARGbytes, ...){
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
socket_ret socketfuncprefix __froot_sckOPENchild(socket_t * socket, UINTN nARGbytes, ...){
	DEBUGPRINT(L"\nSocket Open");
	if(nARGbytes >= 8){
		va_list args;	va_start(args, nARGbytes);
		char *path = va_arg(args, char *);
		char *loadargs = va_arg(args, char *);
		DEBUGPRINT(L"\n\nPath: %p:%a\nLoad-Args: %p:%a", path, path, loadargs, loadargs);
		va_end(args);
		unhandle *uh = __calloc(1, sizeof(unhandle));
		*uh = (unhandle){
			.dir = flagcheck((__ffind((conf_fsroot *)socket->persistent, path))->attr, __fsdirectory),
			.__notype = flagcheck((__ffind((conf_fsroot *)socket->persistent, path))->attr, __fsdirectory)? 
				(void *)floadhdir((conf_fsroot *)socket->persistent, path, loadargs):
					(void *)floadh((conf_fsroot *)socket->persistent, path, loadargs)
		};
		// Open Socket
		socket_t *socket = __calloc(1, sizeof(socket_t));
		*socket = (socket_t){
			.open = (socketOPENchild *)getptr(__fhandle_sckOPENchild),
			.close = (socketCLOSE *)getptr(__fhandle_sckclose),
			.persistent = uh,
			.read = (socketREAD *)getptr(__fhandle_sckread),
			.write = (socketWRITE *)getptr(__fhandle_sckwrite),
			.raw = {
				.read = (socketREAD *)getptr(__fhandle_sckread),
				.write = (socketWRITE *)getptr(__fhandle_sckwrite)
			}
		};
		return (socket_ret){__noerr, sizeof(socket_t), socket};
	}
	return (socket_ret){__incompatible_arg, 0, NULL};
}
socket_ret * socketfuncprefix __froot_sckopen(UINT32 ignore, UINTN nARGbytes, va_list *args){
	DEBUGPRINT(L"\nSocket Open");
	DEBUGPRINT(L"    Opening FS Socket");
	socket_ret *sret = __calloc(1, sizeof(socket_ret));
	if(nARGbytes < sizeof(EFI_GUID)){
		*sret = (socket_ret){.errout = __incompatible_arg, .data = NULL, .nData = 0};
		return sret;
	}
	EFI_GUID guid = va_arg(*args, EFI_GUID), aGuid = va_arg(*args, EFI_GUID);
	conf_fsroot *root = fmount(guid, aGuid);
	if(root){
		socket_t *socket = __calloc(1, sizeof(socket_t));
		*socket = (socket_t){
			.persistent = getptr(root),
			.close = (socketCLOSE *)getptr(__froot_sckclose),
			.open = (socketOPENchild *)getptr(__froot_sckOPENchild),
			.read = (socketREAD *)getptr(__froot_sckread),
			.write = (socketWRITE *)getptr(__froot_sckwrite),
			.raw = {
				.read = (socketREADraw *)getptr(__froot_sckread),
				.write = (socketWRITEraw *)getptr(__froot_sckwrite)
			}
		};
		*sret = (socket_ret){
			.errout = __noerr,
			.data = socket,
			.nData = sizeof(socket)
		};
		return sret;
	}
	*sret = (socket_ret){__incompatible_arg, 0, NULL};
	return sret;
}