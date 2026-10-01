#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH_LEN 1024

// OS-specific path separator for PATH environment variable
#ifdef _WIN32
    #define PATH_SEPARATOR ";"
    #define DIR_SEPARATOR "\\"
#else
    #define PATH_SEPARATOR ":"
    #define DIR_SEPARATOR "/"
#endif

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

#ifdef _WIN32
    #define PATH_SEPARATOR ";"
    #define DIR_SEPARATOR "\\"
#else
    #define PATH_SEPARATOR ":"
    #define DIR_SEPARATOR "/"
#endif

// Check if a file exists at the given path and is a regular file
int file_exists(const char *path){
    struct stat buffer;
    return (stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode));
}

int search_in_path(const char *filename, char *result_path){
    char *path_env = getenv("PATH");
    if(!path_env){return 0;}

    // Duplicate string because strtok modifies it
    char *path_copy = strdup(path_env);
    if(!path_copy){return 0;}

    char *dir = strtok(path_copy, PATH_SEPARATOR);
    while(dir != NULL){
        snprintf(result_path, MAX_PATH_LEN, "%s%s%s", dir, DIR_SEPARATOR, filename);
        if(file_exists(result_path)){
            free(path_copy);
            return 1; // Found
        }
        dir = strtok(NULL, PATH_SEPARATOR);
    }
    free(path_copy);
    return 0;
}

int search_recursive(const char *current_dir, const char *filename, char *result_path){
    DIR *dir = opendir(current_dir);
    if(!dir){return 0;}

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        // Skip "." and ".." to prevent infinite loops
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){continue;}

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, MAX_PATH_LEN, "%s%s%s", current_dir, DIR_SEPARATOR, entry->d_name);

        struct stat statbuf;
        if(stat(full_path, &statbuf) == 0){
            // If it's a directory, recurse into it
            if(S_ISDIR(statbuf.st_mode)){
                if(search_recursive(full_path, filename, result_path)){
                    closedir(dir);
                    return 1;
                }
            }else if (S_ISREG(statbuf.st_mode) && strcmp(entry->d_name, filename) == 0){
                strncpy(result_path, full_path, MAX_PATH_LEN);
                closedir(dir);
                return 1;
            }
        }
    }
    
    closedir(dir);
    return 0;
}

// Main lookup orchestrator
int FileSearch(const char *filename, char *result_path){
    // Step 1: Check PATH
    if(search_in_path(filename, result_path)){return 1;}

    // Step 2: Check CWD and subdirectories
    char cwd[MAX_PATH_LEN];
    if(getcwd(cwd, sizeof(cwd)) != NULL){if(search_recursive(cwd, filename, result_path)){return 1;}}
    return 0; // Not found anywhere
}

char *PoolGetPath(const char *file){
	char temp[2048] = {0};
	if(FileSearch(file, temp)){return strdup(temp);}
	return NULL;
}