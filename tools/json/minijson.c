#include "minijson.h"

// Helper utilities
#define MakeTokenString(TOKEN)      #TOKEN

static inline void skipSpaces(const char** text){
	while((**text == ' ') || (**text == '\n') || (**text == '\t') || (**text == '\r')){(*text)++;}
}

// Helper function to handle character escaping and write raw values out to a stream
static void dumpPrimitiveValue(JsonValue* value, FILE* stream){
    if(!value || !stream){return;}
    switch(value->type){
        case JTYPE_STRING: {
            // Maintain basic string syntax requirements with structural quotes
            fprintf(stream, "\"%s\"", value->stringValue ? value->stringValue : "");
            break;
		} case JTYPE_NUMBER: {
            // Dump raw numeric float or integer values
            fprintf(stream, "%g", value->numberValue);
            break;
		} case JTYPE_BOOL: {
            fprintf(stream, value->boolValue ? "true" : "false");
            break;
		} case JTYPE_NULL: {
            fprintf(stream, "null");
            break;
		} default: {break;}
    }
}

static inline char* readString(const char** text){
	if(**text != '"'){return NULL;}
	(*text)++; 
	const char* start = *text;
	while(**text != '"' && **text != '\0'){(*text)++;}
	int length = *text - start;
	char* result = (char*)malloc(length + 1);
	if(!result){return NULL;}
	strncpy(result, start, length);
	result[length] = '\0';
	if(**text == '"'){(*text)++;} 
	return result;
}

static inline double readNumber(const char** text){
	char* end;
	double num = strtod(*text, &end);
	*text = end;
	return num;
}

static inline bool readBool(const char** text){
	if(strncmp(*text, MakeTokenString(true), sizeof(MakeTokenString(true)) - 1) == 0){
		*text += sizeof(MakeTokenString(true)) - 1;
		return true;
	}else if(strncmp(*text, MakeTokenString(false), sizeof(MakeTokenString(false)) - 1) == 0){
		*text += sizeof(MakeTokenString(true)) - 1;
		return false;
	}
	return 0;
}

// Low-level allocator ensures trackers are zero-initialized
JsonValue* JsonCreateValue(JsonType type){
	JsonValue* val = (JsonValue*)calloc(1, sizeof(JsonValue));
	if(val){val->type = type;}
	return val;
}

static JsonValue* readObject(const char** text){
	JsonValue* obj = JsonCreateValue(JTYPE_OBJECT);
	if(!obj){return NULL;}
	obj->objectValue.capacity = 16;
	obj->objectValue.pairs = malloc(sizeof(JsonPair) * obj->objectValue.capacity);
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
			obj->objectValue.capacity *= 2;
			obj->objectValue.pairs = realloc(obj->objectValue.pairs, sizeof(JsonPair) * obj->objectValue.capacity);
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
	}else if(strncmp(*text, "true", 4) == 0 || strncmp(*text, "false", 5) == 0){
		JsonValue* val = JsonCreateValue(JTYPE_BOOL);
		val->boolValue = readBool(text);
		return val;
	}else if(strncmp(*text, "null", 4) == 0){
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
    sb->buffer = (char*)malloc(sb->capacity);
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
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if(needed < 0){
        va_end(args);
        return;
    }

    // Check if we need to resize the buffer to fit the new string + null terminator
    while(sb->offset + needed >= sb->capacity){
        sb->capacity *= 2;
        char* new_buf = (char*)realloc(sb->buffer, sb->capacity);
        if(!new_buf){
            va_end(args);
            return; // Out of memory
        }
        sb->buffer = new_buf;
    }

    // Perform the actual write into the buffer tracking offset
    vsnprintf(sb->buffer + sb->offset, sb->capacity - sb->offset, format, args);
    sb->offset += needed;
    va_end(args);
}

// Recursive structural worker
static void serialize_worker(JsonValue* value, StringBuilder* sb){
    if(!value){return;}

    switch(value->type){
        case JTYPE_STRING:	{sb_append_format(sb, "\"%s\"", value->stringValue ? value->stringValue : "");	break;}
        case JTYPE_NUMBER:	{sb_append_format(sb, "%g", value->numberValue);								break;}
        case JTYPE_BOOL:	{sb_append_format(sb, value->boolValue ? "true" : "false");						break;}
        case JTYPE_NULL:	{sb_append_format(sb, "null");													break;}
        case JTYPE_OBJECT: {
            sb_append_format(sb, "{");
            for (int i = 0; i < value->objectValue.count; i++) {
                sb_append_format(sb, "\"%s\":", value->objectValue.pairs[i].key ? value->objectValue.pairs[i].key : "");
                
                // Recurse child values directly
                serialize_worker(value->objectValue.pairs[i].value, sb);

                // Add commas separating fields, avoiding trailing syntax violations
                if (i < value->objectValue.count - 1) {
                    sb_append_format(sb, ",");
                }
            }
            sb_append_format(sb, "}");
            break;
		}
    }
}

// Public API function
char* JsonSerialize(JsonValue* root){
    if(!root){return NULL;}

    StringBuilder sb;
    sb_init(&sb, 512); // Start with a safe 512-byte buffer baseline
    
    if(!sb.buffer){return NULL;}
    serialize_worker(root, &sb);

    // Minimize the final buffer size to perfectly fit the generated raw string
    char* final_str = (char*)realloc(sb.buffer, sb.offset + 1);
    if(final_str){return final_str;}
    return sb.buffer; // Fallback to un-minimized buffer if last realloc fails
}

// Hierarchical memory tracking destructor block
void JsonFree(JsonValue* root, char *out){
    if(!root){return;}
	FILE *f = fopen(out, "wb");

    // Phase 1: Output the structure serialization layer
    if(root->type == JTYPE_OBJECT){
        if(f){fprintf(f, "{");}
        for(int i = 0; i < root->objectValue.count; i++){
            // Output the key mapping layout
            if(f){fprintf(f, "\"%s\":", root->objectValue.pairs[i].key ? root->objectValue.pairs[i].key : "");}
            
            // Recurse directly if it's a nested object block
            if(root->objectValue.pairs[i].value && root->objectValue.pairs[i].value->type == JTYPE_OBJECT){
				fclose(f);
                JsonFree(root->objectValue.pairs[i].value, out);
				f = fopen(out, "rb+");
            }else{
                // Otherwise dump the direct primitive node
                dumpPrimitiveValue(root->objectValue.pairs[i].value, f);
                // Track memory cleanup inside the tree layer manually since we aren't recursing primitives
                if(root->objectValue.pairs[i].value){
                    if(root->objectValue.pairs[i].value->type == JTYPE_STRING){
                        free(root->objectValue.pairs[i].value->stringValue);
                    }
                    free(root->objectValue.pairs[i].value);
                }
            }
            
            // Maintain syntax comma delimiters cleanly between array pairs
            if(i < root->objectValue.count - 1){fprintf(f, ",");}
        }
        if(f){fprintf(f, "}");}
    }else{
        // Fallback catch if JsonFree is called directly on an individual primitive root node
        dumpPrimitiveValue(root, stdout);
        if(root->type == JTYPE_STRING){free(root->stringValue);}
    }
    // Phase 2: Clear structural tracking allocations for the current node
    if(root->type == JTYPE_OBJECT){
        for(int i = 0; i < root->objectValue.count; i++){free(root->objectValue.pairs[i].key);}
        free(root->objectValue.pairs);
    }
    free(root);
}

// Get value via standard nested dot path evaluation
JsonValue* JsongetValue(JsonValue* root, const char* path){
	if(root == NULL || root->type != JTYPE_OBJECT){return NULL;}
	char* pathCopy = strdup(path);
	char* token = strtok(pathCopy, ".");
	JsonValue* current = root;
	while(token != NULL && current->type == JTYPE_OBJECT){
		int found = 0;
		for(int i = 0; i < current->objectValue.count; i++){
			if(strcmp(current->objectValue.pairs[i].key, token) == 0){
				current = current->objectValue.pairs[i].value;
				found = 1;
				break;
			}
		}
		if(!found){
			free(pathCopy);
			return NULL;
		}
		token = strtok(NULL, ".");
	}
	free(pathCopy);
	return current;
}

// Master path creation and update engine
int JsonSetValue(JsonValue* root, const char* path, JsonValue* newValue){
	if(!root || root->type != JTYPE_OBJECT || !path || !newValue){return 0;}
	char* pathCopy = strdup(path);
	char* token = strtok(pathCopy, ".");
	JsonValue* current = root;
	
	while(token != NULL){
		char* nextToken = strtok(NULL, ".");
		int foundIndex = -1;
		for(int i = 0; i < current->objectValue.count; i++){
			if(strcmp(current->objectValue.pairs[i].key, token) == 0){
				foundIndex = i;
				break;
			}
		}
		if(nextToken == NULL){
			// Leaf node: perform the write operations
			if(foundIndex != -1){
				// Free previous allocations on value substitution
				JsonFree(current->objectValue.pairs[foundIndex].value, path);
				current->objectValue.pairs[foundIndex].value = newValue;
			}else{
				// Key does not exist: perform a structured push
				if(current->objectValue.count >= current->objectValue.capacity){
					current->objectValue.capacity *= 2;
					current->objectValue.pairs = realloc(current->objectValue.pairs, sizeof(JsonPair) * current->objectValue.capacity);
				}
				current->objectValue.pairs[current->objectValue.count].key = strdup(token);
				current->objectValue.pairs[current->objectValue.count].value = newValue;
				current->objectValue.count++;
			}
			newValue->parent = current;
			
			// Loop to wire sibling pointers inside current hierarchy layer
			if(current->objectValue.count > 1){
				current->objectValue.pairs[current->objectValue.count - 2].value->next = newValue;
			}
			
			free(pathCopy);
			return 1;
		}else{
			// Intermediate node processing path logic
			if(foundIndex != -1){
				if(current->objectValue.pairs[foundIndex].value->type != JTYPE_OBJECT){
					// Force path upgrade to structural object layout if matching primitive intermediate exists
					JsonValue* intermediateObj = JsonCreateValue(JTYPE_OBJECT);
					intermediateObj->objectValue.capacity = 16;
					intermediateObj->objectValue.pairs = malloc(sizeof(JsonPair) * intermediateObj->objectValue.capacity);
					intermediateObj->parent = current;
					JsonFree(current->objectValue.pairs[foundIndex].value, path);
					current->objectValue.pairs[foundIndex].value = intermediateObj;
				}
				current = current->objectValue.pairs[foundIndex].value;
			}else{
				// Missing dynamic tree branch; construct the intermediate object layout map
				JsonValue* intermediateObj = JsonCreateValue(JTYPE_OBJECT);
				intermediateObj->objectValue.capacity = 16;
				intermediateObj->objectValue.pairs = malloc(sizeof(JsonPair) * intermediateObj->objectValue.capacity);
				intermediateObj->parent = current;
				if(current->objectValue.count >= current->objectValue.capacity){
					current->objectValue.capacity *= 2;
					current->objectValue.pairs = realloc(current->objectValue.pairs, sizeof(JsonPair) * current->objectValue.capacity);
				}
				current->objectValue.pairs[current->objectValue.count].key = strdup(token);
				current->objectValue.pairs[current->objectValue.count].value = intermediateObj;
				current->objectValue.count++;
				current = intermediateObj;
			}
		}
		token = nextToken;
	}
	free(pathCopy);
	return 0;
}

// Wrapper APIs for Primitives
int JsonSetString(JsonValue* root, const char* path, const char* value){
	JsonValue* val = JsonCreateValue(JTYPE_STRING);
	val->stringValue = strdup(value);
	return JsonSetValue(root, path, val);
}

int JsonSetNumber(JsonValue* root, const char* path, double value){
	JsonValue* val = JsonCreateValue(JTYPE_NUMBER);
	val->numberValue = value;
	return JsonSetValue(root, path, val);
}

int JsonSetBool(JsonValue* root, const char* path, int value){
	JsonValue* val = JsonCreateValue(JTYPE_BOOL);
	val->boolValue = value;
	return JsonSetValue(root, path, val);
}

int JsonSetNull(JsonValue* root, const char* path){
	return JsonSetValue(root, path, JsonCreateValue(JTYPE_NULL));
}

int JsonSetObject(JsonValue* root, const char* path){
	JsonValue* val = JsonCreateValue(JTYPE_OBJECT);
	val->objectValue.capacity = 16;
	val->objectValue.pairs = malloc(sizeof(JsonPair) * val->objectValue.capacity);
	return JsonSetValue(root, path, val);
}

char* loadFile(const char* filename){
	FILE* f = fopen(filename, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);
	char* data = malloc(size + 1);
	if (data) {
		size_t readBytes = fread(data, 1, size, f);
		data[readBytes] = '\0';
	}
	fclose(f);
	return data;
}

void printValue(JsonValue* value){
	if(!value){
		switch(value->type){
			case JTYPE_STRING: {printf("[String]\"%s\"\n", value->stringValue);						break;}
			case JTYPE_NUMBER: {printf("[Number](%.2f)\n", value->numberValue);						break;}
			case JTYPE_BOOL:   {printf("[Boolean]\"%s\"\n", value->boolValue ? "true" : "false");	break;}
			case JTYPE_NULL:   {printf("(Null)\n");													break;}
			default:           {printf("[Object](%d Sub-Objects)\n", value->objectValue.count);		break;}
		}
	}else{printf("Not found\n");}
}