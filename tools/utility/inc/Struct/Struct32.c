#include "Struct32.h"

treeroot *TCreate(uint16_t baseSize, bool UseSPT){
	treeroot *out = calloc(1, sizeof(treeroot));
	*out = (treeroot){
		.useSPT = UseSPT,
		.collapse = rand(),
		.SPT = {
			.len = 0,
			.SuperLookupTable = malloc(sizeof(SuperLookupTableEntry))
		},
		.mbase = baseSize,
		.tree = Ncreate(NULL, NULL, NULL, 0)
	};
	return out;
}
node *TSearch(treeroot *r, char *Path){
	uint32_t ID = FhashCrunch(Path, strlen(Path), r->collapse);
	for(uint32_t cc = 0; cc < r->tree.children.numchildren; ++cc){
		if(r->tree.children.children[cc].ID == ID){
			return r->tree.children.children[cc];
		}
	}
}

static uint32_t nodeID;
node *Ncreate(node *parent, char *name, void *value, uint32_t vallen){
	node *out = calloc(1, sizeof(node));
	*out = (node){
		.code = nodeID,
		.valLength = vallen,
		.value = value,
		.parent = FhashCrunch(),
		.ID = name,
		.children = {
			.children = NULL,
			.numchildren = 0
		}
	};
	nodeID++;
	return out;
}
void TRemoveChildN(treeroot *r, node *n){
	for(uint32_t cc = 0; cc < r->tree.children.numchildren; ++cc){
		if(r->tree.children.children[cc].code == n->code){
			memcpy(r->tree.children.children + cc - 1, r->tree.children.children + cc, sizeof(node));
			r->tree.children.numchildren--;
			r->tree.children.children = realloc(r->tree.children.children, sizeof(node) * r->tree.children.numchildren);
		}
	}
}
bool NremoveChildN(node *parent, node *child){return NremoveChild(parent, child->code);}
bool NremoveChild(node *parent, ID code){
	for(uint32_t cc = 0; cc < parent->children.numchildren; ++cc){
		if(parent->children.children[cc].code == code){
			memcpy(parent->children.children + cc - 1, parent->children.children + cc, sizeof(node));
			parent->children.numchildren--;
			parent->children.children = realloc(parent->children.children, sizeof(node));
			return true;
		}
	}
	return false;
}

node *NaddChild(treeroot *root, node *parent, char *name, void *value, uint32_t vallen){
	node *out = Ncreate(parent, name, value, vallen);
	NaddChildN(root, parent, out);
	return out;
}

void NaddChildN(treeroot *root, node *parent, node *child){
	if(child->parent){NremoveChildN(child->parent, child->code);}
	if(parent->children.children){parent->children.children = realloc(parent->children.children, sizeof(node) * (parent->children.numchildren + 1));}
	else{parent->children.children = malloc(sizeof(node));}
	child->code = deriveCode(root, parent, parent->children.numchildren);
	parent->children.children[parent->children.numchildren] = *child;
	parent->children.numchildren++;
}

void TAddChildN(treeroot *r, node *n){
	if(r->tree.children.numchildren){
		r->tree.children.children = realloc(r->tree.children.children, sizeof(node) * (r->tree.children.numchildren + 1));
	}else{r->tree.children.children = malloc(sizeof(node));		r->tree.children.numchildren = 1;}
	TRemoveChildN(r, n);
	r->tree.children.children[r->tree.children.numchildren - 1] = *n;
}

ID deriveCode(treeroot *root, node *parent, uint32_t childIndex){return (parent->code * root->mbase) + childIndex;}

ID depth(node *n, treeroot *r, uint32_t targetDepth){
    uint32_t d = 0;
    ID c = n->code;

    // compute full depth
    while(c >= r->mbase){c /= r->mbase;		d++;}

    // remove suffix digits until we reach targetDepth
    uint32_t remove = d - targetDepth;
	ID code_ = n->code;
    while(remove--){code_ /= r->mbase;}

    return code_;
}

moveProgress *genProgress(treeroot *root, node *a, node *b){
	moveProgress *out = calloc(1, sizeof(moveProgress));
	*out = (moveProgress){
		.done = false,
		.maxDepth = __max(depth(a, root, 0), depth(b, root, 0)),
		.root = root,
		.step = 0
	};
	return out;
}
MoveInstr step(moveProgress *p){
    if(p->done){return MOVE_FINISHED;}

    // compute prefixes at current step
    ID Ap = depth(p->A->code, p->root->mbase, p->step);
    ID Bp = depth(p->B->code, p->root->mbase, p->step);

    // if prefixes match, keep going deeper
    if(Ap == Bp){
        p->step++;
        if(p->step > p->maxDepth){p->done = 1;	return MOVE_FINISHED;}
        return MOVE_NONE; // no movement yet
    }

    // divergence found at depth p->step
    uint32_t depthA = 0, depthB = 0;
    ID tA = p->A->code, tB = p->B->code;

    while(tA >= p->root->mbase){tA /= p->root->mbase;	depthA++;}
    while(tB >= p->root->mbase){tB /= p->root->mbase;	depthB++;}

    // vertical movement
    if (depthA > p->step){return MOVE_UP;}
    if (depthB > p->step){return MOVE_DOWN;}

    // horizontal movement
    int childA = (p->A->code / (ID)pow(p->root->mbase, depthA - p->step)) % p->root->mbase;
    int childB = (p->A->code / (ID)pow(p->root->mbase, depthB - p->step)) % p->root->mbase;

    if (childA < childB){return MOVE_RIGHT;}
    if (childA > childB){return MOVE_LEFT;}

    p->done = 1;
    return MOVE_FINISHED;
}

node *TSearch(){

}