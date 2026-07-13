
// // Source: "https://medium.com/@priyanshugrv/building-a-simple-json-parser-in-c-9ecd1c6b1b9e"
// #include "minijson.h"

// void skipSpaces(const char** text){
// 	while(**text == ' ' || **text == '\n' || **text == '\t'){(*text)++;}
// }

// char* readString(const char** text){
// 	if(**text != '"'){return NULL;}
// 	(*text)++;  // Skip first quote
// 	const char* start = *text;
// 	while(**text != '"' && **text != '\0'){
// 		(*text)++;
// 	}
// 	int length = *text - start;
// 	char* result = (char*)malloc(length + 1);
// 	strncpy(result, start, length);
// 	result[length] = '\0';
// 	if(**text == '"'){(*text)++;}  // Skip closing quote
// 	return result;
// }

// double readNumber(const char** text){
// 	char* end;
// 	double num = strtod(*text, &end);
// 	*text = end;
// 	return num;
// }

// // Read true/false
// int readBool(const char** text){
// 	if(strncmp(*text, "true", 4) == 0){
// 		*text += 4;
// 		return 1;
// 	}else if(strncmp(*text, "false", 5) == 0){
// 		*text += 5;
// 		return 0;
// 	}
// 	return 0;
// }

// // Read value (string, number, bool, null, object)
// JsonValue* JsonreadValue(const char** text);

// // Read object like { "key": value, ... }
// JsonValue* readObject(const char** text){
// 	JsonValue* obj = (JsonValue*)malloc(sizeof(JsonValue));
// 	obj->type = JTYPE_OBJECT;
// 	obj->objectValue.pairs = malloc(sizeof(JsonPair) * 20);
// 	obj->objectValue.count = 0;
// 	(*text)++;  // Skip '{'
// 	skipSpaces(text);
// 	while(**text != '}' && **text != '\0'){
// 		skipSpaces(text);
// 		char* key = readString(text);
// 		skipSpaces(text);
// 		if (**text == ':') (*text)++;
// 		skipSpaces(text);
// 		JsonValue* value = readValue(text);
// 		obj->objectValue.pairs[obj->objectValue.count].key = key;
// 		obj->objectValue.pairs[obj->objectValue.count].value = value;
// 		obj->objectValue.count++;
// 		skipSpaces(text);
// 		if(**text == ','){
// 			(*text)++;
// 			skipSpaces(text);
// 		}
// 	}
// 	if(text == '}'){(*text)++;}  // Skip '}'
// 	return obj;
// }

// // Read any value
// JsonValue* JsonreadValue(const char** text){
// 	skipSpaces(text);
// 	if(**text == '"'){
// 		JsonValue* val = malloc(sizeof(JsonValue));
// 		val->type = JTYPE_STRING;
// 		val->stringValue = readString(text);
// 		return val;
// 	}else if(isdigit(**text) || **text == '-'){
// 		JsonValue* val = malloc(sizeof(JsonValue));
// 		val->type = JTYPE_NUMBER;
// 		val->numberValue = readNumber(text);
// 		return val;
// 	}else if(strncmp(*text, "true", 4) == 0 || strncmp(*text, "false", 5) == 0){
// 		JsonValue* val = malloc(sizeof(JsonValue));
// 		val->type = JTYPE_BOOL;
// 		val->boolValue = readBool(text);
// 		return val;
// 	}else if (strncmp(*text, "null", 4) == 0){
// 		JsonValue* val = malloc(sizeof(JsonValue));
// 		val->type = JTYPE_NULL;
// 		*text += 4;
// 		return val;
// 	}else if(**text == '{'){return readObject(text);}
// 	return NULL;
// }

// // Load file into string
// char* loadFile(const char* filename){
// 	FILE* f = fopen(filename, "r");
// 	if(!f){return NULL;}
// 	fseek(f, 0, SEEK_END);
// 	long size = ftell(f);
// 	rewind(f);
// 	char* data = malloc(size + 1);
// 	fread(data, 1, size, f);
// 	data[size] = '\0';
// 	fclose(f);
// 	return data;
// }

// // Get value by path like "details.name"
// JsonValue* JsongetValue(JsonValue* root, const char* path){
// 	if(root == NULL || root->type != JTYPE_OBJECT){return NULL;}
// 	char* pathCopy = strdup(path);
// 	char* token = strtok(pathCopy, ".");
// 	JsonValue* current = root;
// 	while(token != NULL && current->type == JTYPE_OBJECT){
// 		int found = 0;
// 		for(int i = 0; i < current->objectValue.count; i++){
// 			if(strcmp(current->objectValue.pairs[i].key, token) == 0){
// 				current = current->objectValue.pairs[i].value;
// 				found = 1;
// 				break;
// 			}
// 		}
// 		if(!found){
// 			free(pathCopy);
// 			return NULL;
// 		}
// 		token = strtok(NULL, ".");
// 	}
// 	free(pathCopy);
// 	return current;
// }

// // Print value
// void printValue(JsonValue* value){
// 	if(!value){
// 		printf("Not found\n");
// 		return;
// 	}
// 	switch(value->type){
// 		case JTYPE_STRING:	{printf("String: %s\n", value->stringValue);					break;}
// 		case JTYPE_NUMBER:	{printf("Number: %.2f\n", value->numberValue);					break;}
// 		case JTYPE_BOOL:		{printf("Boolean: %s\n", value->boolValue ? "true" : "false");	break;}
// 		case JTYPE_NULL:		{printf("Null\n");												break;}
// 		default:			{printf("Object {...}\n");										break;}
// 	}
// }