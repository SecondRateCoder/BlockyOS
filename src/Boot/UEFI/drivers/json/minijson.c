
// Source: "https://medium.com/@priyanshugrv/building-a-simple-json-parser-in-c-9ecd1c6b1b9e"
#include "minijson.h"

MiniJsonFPrefix void skipSpaces(const char** text){
	while(**text == ' ' || **text == '\n' || **text == '\t'){(*text)++;}
}

MiniJsonFPrefix char* readString(const char** text){
	if(**text != '"'){return NULL;}
	(*text)++;  // Skip first quote
	const char* start = *text;
	while(**text != '"' && **text != '\0'){
		(*text)++;
	}
	int length = *text - start;
	char* result = (char*)AllocateZeroPool(length + 1);
	__memcpy(result, start, length);
	result[length] = '\0';
	if(**text == '"'){(*text)++;}  // Skip closing quote
	return result;
}

MiniJsonFPrefix double readNumber(const char** text){
	double num = __strtod(*text);
    while(isdigit(*(*text))){(*text)++;}
	return num;
}

// Read true/false
MiniJsonFPrefix int readBool(const char** text){
	if(__memcmp(*text, "true", 4) == 0){
		*text += 4;
		return 1;
	}else if(__memcmp(*text, "false", 5) == 0){
		*text += 5;
		return 0;
	}
	return 0;
}

MiniJsonFPrefix JsonValue* readValue(const char** text);

MiniJsonFPrefix JsonValue* readObject(const char** text){
	JsonValue* obj = (JsonValue*)AllocateZeroPool(sizeof(JsonValue));
	obj->type = JTYPE_OBJECT;
	obj->objectValue.pairs = AllocateZeroPool(sizeof(JsonPair) * 20);
	obj->objectValue.count = 0;
	(*text)++;  // Skip '{'
	skipSpaces(text);
	while(**text != '}' && **text != '\0'){
		skipSpaces(text);
		char* key = readString(text);
		skipSpaces(text);
		if (**text == ':') (*text)++;
		skipSpaces(text);
		JsonValue* value = readValue(text);
		obj->objectValue.pairs[obj->objectValue.count].key = key;
		obj->objectValue.pairs[obj->objectValue.count].value = value;
		obj->objectValue.count++;
		skipSpaces(text);
		if(**text == ','){
			(*text)++;
			skipSpaces(text);
		}
	}
	if(text == '}'){(*text)++;}  // Skip '}'
	return obj;
}

// Read any value
JsonValue* readValue(const char** text){
	skipSpaces(text);
	if(**text == '"'){
		JsonValue* val = AllocateZeroPool(sizeof(JsonValue));
		val->type = JTYPE_STRING;
		val->stringValue = readString(text);
		return val;
	}else if(isdigit(**text) || **text == '-'){
		JsonValue* val = AllocateZeroPool(sizeof(JsonValue));
		val->type = JTYPE_NUMBER;
		val->numberValue = readNumber(text);
		return val;
	}else if(__memcmp(*text, "true", 4) == 0 || __memcmp(*text, "false", 5) == 0){
		JsonValue* val = AllocateZeroPool(sizeof(JsonValue));
		val->type = JTYPE_BOOL;
		val->boolValue = readBool(text);
		return val;
	}else if(__memcmp(*text, "null", 4) == 0){
		JsonValue* val = AllocateZeroPool(sizeof(JsonValue));
		val->type = JTYPE_NULL;
		*text += 4;
		return val;
	}else if(**text == '{'){return readObject(text);}
	return NULL;
}

// Get value by path like "details.name"
JsonValue* getValue(JsonValue* root, const char* path){
	if(root == NULL || root->type != JTYPE_OBJECT){return NULL;}
	char* pathCopy = strdup(path);
	char* token = strtok(pathCopy, ".");
	JsonValue* current = root;
	while(token != NULL && current->type == JTYPE_OBJECT){
		int found = 0, len = 0;
		for(int i = 0; i < current->objectValue.count; i++){
            if((len = strlena(token)) != strlena(current->objectValue.pairs[i].key)){continue;}
			if(__memcmp(current->objectValue.pairs[i].key, token, len) == 0){
				current = current->objectValue.pairs[i].value;
				found = 1;
				break;
			}
		}
		if(!found){
			FreePool(pathCopy);
			return NULL;
		}
		token = strtok(NULL, ".");
	}
	FreePool(pathCopy);
	return current;
}

// Print value
void printValue(JsonValue* value){
	if(!value){
		Print("Not found\n");
		return;
	}
	switch(value->type){
		case JTYPE_STRING:	{Print("String: %s\n", value->stringValue);					    break;}
		case JTYPE_NUMBER:	{Print("Number: %.2f\n", value->numberValue);					break;}
		case JTYPE_BOOL:		{Print("Boolean: %s\n", value->boolValue ? "true" : "false");	break;}
		case JTYPE_NULL:		{Print("Null\n");												break;}
		default:			{Print("Object {...}\n");										break;}
	}
}