#include "./src/Core/RAM/memtypes.h"
#include <./src/Core/General.h>

signed int program_load(uint8_t *program, size_t size);
void program_run(ID_t ID);

typedef struct funcdependencytree{
    //The address to the father function.
    uint8_t *funcaddr;
    //The size of the function(in bytes.), being it's full, compiled,, executable, binary representation.
    size_t funcsize;
    //The addresses in the father function where the dependency functions are called.
    uint8_t **oldfuncpointers;
    //The size of the old_funcpiinters array.
    size_t size_oldfuncpointers;
    //The address of the funcdependencytrees of the dependency functions.
    uint8_t **dependenciesaddr_array;
    //The number of dependencies.
    size_t numofdependencies;
}funcdependencytree;