#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
    #define PATH_SEP "\\"
    #define ENV_SEP ";"
#else
    #define PATH_SEP "/"
    #define ENV_SEP ":"
#endif

// Your internal fallback directory pool
const char* G_LocalSearchPool[] = {
    ".", 
    "./libs",
    NULL
};

char* PoolGetPath(const char* fileSnippet){
    FILE* f = NULL;
    static char fullPath[2048];

    // 1. Try to open the snippet exactly as provided (Absolute or explicit relative path)
    if((f = fopen(fileSnippet, "rb"))){
        fclose(f);
        return fileSnippet;
    }

    // 2. Walk through your internal Local Search Pool
    int i = 0;
    while(G_LocalSearchPool[i] != NULL){
        snprintf(fullPath, sizeof(fullPath), "%s%s%s", G_LocalSearchPool[i], PATH_SEP, fileSnippet);
        if((f = fopen(fullPath, "rb"))){
            fclose(f);
            return fullPath; 
        }
        i++;
    }

    // 3. Dynamic Environment Variable Pooling (Splitting and searching the system PATH)
    char* rawEnvPath = getenv("PATH");
    if(rawEnvPath){
        // Duplicate the environment string because strtok modifies the source string
        char* envCopy = strdup(rawEnvPath);
        if(envCopy){
            // Grab the first directory from the environment variable string
            char* dirToken = strtok(envCopy, ENV_SEP);
            while(dirToken != NULL){
                // Combine the current environment directory token with our file snippet
                snprintf(fullPath, sizeof(fullPath), "%s%s%s", dirToken, PATH_SEP, fileSnippet);
                if((f = fopen(fullPath, "rb"))){
                    // Found it, Break the loop early
                    break;
                }
                // Get the next directory path in the environment sequence
                dirToken = strtok(NULL, ENV_SEP);       memset(fullPath, 0, sizeof(fullPath));
            }
            free(envCopy); // Clean up the heap-allocated copy
            if(f){fclose(f);    return fullPath;} // Return the valid file handle if found inside the environment loop
        }
    }
    // Completely unresolved across all pools
    return NULL; 
}


char *GenericError(char *str, uint32_t val){
	typedef struct keyval{char *error; uint32_t alias;}keyval;
	static keyval *errors = NULL;
	static uint32_t nErrors = 0;
	if(str && val){
		if(errors){
			nErrors++;
			errors = realloc(errors, sizeof(keyval) * nErrors);
		}else{
			nErrors = 1;
			errors = calloc(1, sizeof(keyval));
		}
		errors[nErrors - 1] = (keyval){.alias = val, .error = strdup(str)};
		return (void *)0x1;
	}else if(!str && val){
		for(uint32_t cc = 0; cc < nErrors; ++cc){
			if(val == errors[cc].alias){return errors[cc].error;}
		}
		return NULL;
	}
	return NULL;
}