#include "minijson.h"
#include <stdarg.h>

// Helper utilities
#define MakeTokenString(TOKEN)      #TOKEN

static inline void skipSpaces(const char** text){
	while((**text == ' ') || (**text == '\n') || (**text == '\t') || (**text == '\r')){(*text)++;}
}

static inline char* readString(const char** text){
	if(**text != '"'){return NULL;}
	(*text)++; 
	const char* start = *text;
	while(**text != '"' && **text != '\0'){(*text)++;}
	int length = *text - start;
	char* result = (char*)__calloc(1, length + 1);
	if(!result){return NULL;}
	__memcpy(result, start, length);
	result[length] = '\0';
	if(**text == '"'){(*text)++;} 
	return result;
}

static inline double readNumber(const char** text){
	char* end;
	double num = __strtod(*text, &end);
	*text = end;
	return num;
}

static inline bool readBool(const char** text){
	if(strncmpa(*text, MakeTokenString(true), sizeof(MakeTokenString(true)) - 1) == 0){
		*text += sizeof(MakeTokenString(true)) - 1;
		return true;
	}else if(strncmpa(*text, MakeTokenString(false), sizeof(MakeTokenString(false)) - 1) == 0){
		*text += sizeof(MakeTokenString(true)) - 1;
		return false;
	}
	return 0;
}

// Low-level allocator ensures trackers are zero-initialized
JsonValue* JsonCreateValue(JsonType type){
	JsonValue* val = (JsonValue*)__calloc(1, sizeof(JsonValue));
	if(val){val->type = type;}
	return val;
}

static JsonValue* readObject(const char** text){
	JsonValue* obj = JsonCreateValue(JTYPE_OBJECT);
	if(!obj){return NULL;}
	obj->objectValue.capacity = 16;
	obj->objectValue.pairs = __calloc(sizeof(JsonPair), obj->objectValue.capacity);
	obj->objectValue.count = 0;
	
	//  Skip '{'
	(*text)++;
	skipSpaces(text);
	JsonValue* lastChild = NULL;
	while(**text != '}' && **text != '\0'){
		skipSpaces(text);
		if(**text == '}'){break;}

		char* key = readString(text);
		skipSpaces(text);
		if(**text == ':'){(*text)++;}
		skipSpaces(text);
		
		JsonValue* value = JsonreadValue(text);
		if(value){
			//  Wire parent tracking
			value->parent = obj;
			//  Wire sequential sibling tracking
			if(lastChild){lastChild->next = value;}
			lastChild = value;
		}
		
		//	Dynamically grow dictionary array capacity if required
		if(obj->objectValue.count >= obj->objectValue.capacity){
			obj->objectValue.pairs = __realloc(obj->objectValue.pairs, 
				sizeof(JsonPair) * obj->objectValue.capacity, sizeof(JsonPair) * obj->objectValue.capacity * 2);
			obj->objectValue.capacity *= 2;
		}
		obj->objectValue.pairs[obj->objectValue.count].key = key;
		obj->objectValue.pairs[obj->objectValue.count].value = value;
		obj->objectValue.count++;
		
		skipSpaces(text);
		if(**text == ','){
			(*text)++;
			skipSpaces(text);
		}
	}
	if(**text == '}'){(*text)++;} 
	return obj;
}

JsonValue* JsonreadValue(const char** text){
	skipSpaces(text);
	if(**text == '"'){
		JsonValue* val = JsonCreateValue(JTYPE_STRING);
		val->stringValue = readString(text);
		return val;
	}else if(isdigit(**text) || **text == '-'){
		JsonValue* val = JsonCreateValue(JTYPE_NUMBER);
		val->numberValue = readNumber(text);
		return val;
	}else if(strncmpa(*text, "true", 4) == 0 || strncmpa(*text, "false", 5) == 0){
		JsonValue* val = JsonCreateValue(JTYPE_BOOL);
		val->boolValue = readBool(text);
		return val;
	}else if(strncmpa(*text, "null", 4) == 0){
		JsonValue* val = JsonCreateValue(JTYPE_NULL);
		*text += 4;
		return val;
	}else if(**text == '{'){return readObject(text);}
	return NULL;
}
JsonValue* JsonParse(const char* text){return JsonreadValue(&text);}

// Initialize the string builder on the heap
static void sb_init(StringBuilder* sb, size_t initial_capacity){
    sb->capacity = initial_capacity > 0 ? initial_capacity : 256;
    sb->buffer = (char*)__calloc(1, sb->capacity);
    sb->offset = 0;
    if(sb->buffer){sb->buffer[0] = '\0';}
}

// Safely append a formatted string, growing the buffer dynamically if needed
static void sb_append_format(StringBuilder* sb, const char* format, ...){
    va_list args;
    va_start(args, format);
    
    // Copy va_list to safely measure length without consuming the args
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = AsciiVSPrint(NULL, 0, format, args_copy);
    va_end(args_copy);
    if(needed < 0){
        va_end(args);
        return;
    }

    // Check if we need to resize the buffer to fit the new string + null terminator
    while(sb->offset + needed >= sb->capacity){
        char* new_buf = (char*)__realloc(sb->buffer, sb->capacity, sb->capacity * 2);
        sb->capacity *= 2;
        if(!new_buf){
            va_end(args);
            return; // Out of memory
        }
        sb->buffer = new_buf;
    }

    // Perform the actual write into the buffer tracking offset
    AsciiVSPrint(sb->buffer + sb->offset, sb->capacity - sb->offset, format, args);
    sb->offset += needed;
    va_end(args);
}

// Hierarchical memory tracking destructor block
void JsonFree(JsonValue* root){
    if(!root){return;}

    // Phase 1: Output the structure serialization layer
    if(root->type == JTYPE_OBJECT){
        for(int i = 0; i < root->objectValue.count; i++){
            
            // Recurse directly if it's a nested object block
            if(root->objectValue.pairs[i].value && root->objectValue.pairs[i].value->type == JTYPE_OBJECT){
                JsonFree(root->objectValue.pairs[i].value);
            }else{
                // Track memory cleanup inside the tree layer manually since we aren't recursing primitives
                if(root->objectValue.pairs[i].value){
                    if(root->objectValue.pairs[i].value->type == JTYPE_STRING){
                        __free(root->objectValue.pairs[i].value->stringValue);
                    }
                    __free(root->objectValue.pairs[i].value);
                }
            }
        }
    }else{
        if(root->type == JTYPE_STRING){__free(root->stringValue);}
    }
    // Phase 2: Clear structural tracking allocations for the current node
    if(root->type == JTYPE_OBJECT){
        for(int i = 0; i < root->objectValue.count; i++){__free(root->objectValue.pairs[i].key);}
        __free(root->objectValue.pairs);
    }
    __free(root);
}

// Get value via standard nested dot path evaluation
JsonValue* JsongetValue(JsonValue* root, const char* path){
	if(root == NULL || root->type != JTYPE_OBJECT){return NULL;}
    strtok_t *tstate = strtok_i(path, ".", 0x00);
	char* token = strtok_k(tstate);
	JsonValue* current = root;
	while(token != NULL && current->type == JTYPE_OBJECT){
		int found = 0;
		for(int i = 0; i < current->objectValue.count; i++){
			if(AsciiStrCmp(current->objectValue.pairs[i].key, token) == 0){
				current = current->objectValue.pairs[i].value;
				found = 1;
				break;
			}
		}
		if(!found){return NULL;}
		token = strtok_k(tstate);
	}
	return current;
}