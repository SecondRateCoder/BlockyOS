#include "./src/Public/Publics.h"


int main(char *command, size_t len){
    size_t cc =0;
    char *path = alloca(MAXNAME_LENGTH, (uint8_t[8]){0}); 
    strslice_till(command, path, ' ', MAXNAME_LENGTH);
    while(cc < numofpaths){
        if(strcompare_l(Paths[cc].Name, path, MAXNAME_LENGTH) == true){
            //Run function.
        }
    }
}