

int prog_load(char *path, ...){
    int *argp = (int *)&path;
    argp += sizeof(path) / sizeof(int);
    argp++;
}