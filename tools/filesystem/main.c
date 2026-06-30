#include "frat.h"
#include "shell.h"
#include <signal.h>

void crash(int sig){
	char *_sig = NULL;
	switch(sig){
		case SIGSEGV: 	_sig = "SEG FAULT";				break;
		case SIGABRT: 	_sig = "ABORT";					break;
		case SIGFPE:	_sig = "Zero-Div";				break;
		case SIGILL:	_sig = "Illegal Instruction";	break;
		case SIGTERM:	_sig = "Program Termination";	break;
		default:		_sig = "UNKNOWN";				break;
	};
	fprintf(stderr, "\n\n=== PROGRAM CRASHED ===\n");
	fprintf(stderr, "Signal: %d (%s)\n", sig, _sig);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

void init_sighandler(void){
	signal(SIGSEGV, crash); // segmentation fault
	signal(SIGABRT, crash); // abort()
	signal(SIGFPE,  crash); // divide by zero
	signal(SIGILL,  crash); // illegal instruction
	signal(SIGTERM, crash); // termination request
}

int main(int argc, char *argv[]){
	printf("\nFSFRAT DRIVER");
	init_sighandler();
	if(argc > 1){
		// There's a Command to Parse
		for(uint32_t cc = 1; cc < argc; cc++){printf("\n  %s", argv[cc]);}
		for(uint32_t cc = 1; cc < argc; cc++){
			printf("\n%s", argv[cc]);
			cmd_errout info = __shellparse(argv[cc]);
			printf("\t[%u]: \"%s\"", info.errcode, info.msg);
		}
		fflush(stdout);
		exit(EXIT_SUCCESS);
	}else{
		char *logfile = strdup(getcwd(NULL, 0));		uint32_t logn = strlen(logfile);
		logfile = realloc(logfile, (logn + strlen(LOGFOFFSET) + 2) * sizeof(char));
		logfile[logn] = '\\';
		memcpy(logfile + logn + 1, LOGFOFFSET, strlen(LOGFOFFSET) + 1);
		if(!(logf = fopen(logfile, "w"))){exit(EXIT_FAILURE);}
		free(logfile);
		char *temp = calloc(64, sizeof(char));
		time_t t;			time(&t);
		struct tm *t_ = calloc(1, sizeof(struct tm));
#if defined(_WIN32) || defined(_WIN64)
		// Windows safe version (arguments are inverted)
		localtime_s(t_, &t);
#else
		// POSIX (Linux/macOS) safe version
		localtime_r(t_, &t);
#endif
		snprintf(temp, 64, "\n[%u:    :%u:    :%u:    :%u:    :%u:    :%u]", 
			t_->tm_yday, t_->tm_mon, t_->tm_wday, t_->tm_hour, t_->tm_min, t_->tm_sec
		);
		fwrite(temp, sizeof(char), strlen(temp), logf);				fflush(logf);
		free(t_);
		do{
			char *bf = readbuf(0, "\n>> ");
			fwrite("\n", sizeof(char), 1, logf);
			fwrite(bf, sizeof(char), strlen(bf), logf);
			fflush(logf);
			if(bf && (strlen(bf) > 2)){
				cmd_errout info = __shellparse(bf);
				if(info.errcode){dualprintf(logf, stdout, "\n[%u]: \"%s\"", info.errcode, info.msg);}
				free(bf);
			}
		}while(true);
	}
	dualprintf(logf, stdout, "\nNo Commands");
	fflush(stdout);
	exit(EXIT_FAILURE);
}