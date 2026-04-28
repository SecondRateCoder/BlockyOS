#include "tools/utility/util.h"

typedef uint64_t ID;
// #define base 4294967295

typedef struct SuperLookupTableEntry{
	node *ptr;
	uint32_t namehash;
}SuperLookupTableEntry;
bufdef(SuperLookupTableEntry, SuperLookupTable, uint32_t);
typedef struct treeroot{
	bool useSPT;
	uint8_t collapse;
	uint16_t mbase;
	SuperLookupTable SPT;
	node tree;
}treeroot;

typedef struct node{
	uint32_t valLength;
	ID code;
	void *value;
	uint32_t ID;
	struct node *parent;
	struct tree children;
}PACKEDSTRUCT node;

typedef struct tree{
	node *children;
	uint32_t numchildren;
}tree;

typedef struct moveProgress{
	treeroot *root;
	node *A, *B;
    uint32_t step;
    uint32_t maxDepth;
    int done;
}moveProgress;

defenum(uint8_t, MoveInstr){
    MOVE_NONE,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_FINISHED
};

typedef enum encryptInstr{
	SHL = 0x1,
	SHR = 0x2,
	DWORDSWAP = 0x4,
	HIGHSWAP = 0x8,
	LOWSWAP = 0x10,
}encryptInstr;

typedef struct ptrEncrypt{
	union{
		void *ptr;
		struct{
			uint16_t hh,
					 hl,
					 lh,
					 ll;
		};
	};
	uint16_t seed;
}ptrEncrypt;

#define TReplaceChildN TAddChildN
#define ReplaceChildN TAddChildN

void TAddChildN(treeroot *r, node *n);
void TRemoveChildN(treeroot *r, node *n);
treeroot *TCreate(uint16_t baseSize, bool UseSPT);
node *Ncreate(node *parent, char *name, void *value, uint32_t vallen);
bool NremoveChildN(node *parent, node *child);
bool NremoveChild(node *parent, ID code);
node *NaddChild(treeroot *root, node *parent, char *name, void *value, uint32_t vallen);
void NaddChildN(treeroot *root, node *parent, node *child);
ID deriveCode(treeroot *root, node *parent, uint32_t childIndex);
ID depth(node *n, treeroot *r, uint32_t targetDepth);
moveProgress *genProgress(treeroot *root, node *a, node *b);
MoveInstr step(moveProgress *p);

uint32_t FhashCrunch(void *buffer, size_t bufflen, uint8_t collapse);
uint32_t hashCrunch(uint8_t buffer[SHA256_SIZE_BYTES], uint8_t collapse);