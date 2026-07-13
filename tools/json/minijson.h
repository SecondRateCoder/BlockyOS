#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum{
    JTYPE_STRING,
    JTYPE_NUMBER,
    JTYPE_BOOL,
    JTYPE_NULL,
    JTYPE_OBJECT
}JsonType;

struct JsonValue;

typedef struct JsonPair{
    char* key;
    struct JsonValue* value;
}JsonPair;

typedef struct JsonValue{
    JsonType type;
    struct JsonValue* parent;
    struct JsonValue* next;
    union{
        char* stringValue;
        double numberValue;
        int boolValue;
        struct{
            JsonPair* pairs;
            int count;
            int capacity;
        }objectValue;
    };
}JsonValue;

// Internal structure to handle safe dynamic string appending
typedef struct{
    char* buffer;
    size_t offset;
    size_t capacity;
}StringBuilder;

// Parser Core
JsonValue* JsonParse(const char* text);
JsonValue* JsonreadValue(const char** text);
void JsonFree(JsonValue* root, char *out);

// Node Allocators
JsonValue* JsonCreateValue(JsonType type);

// Path resolution and management suite
JsonValue* JsongetValue(JsonValue* root, const char* path);
int JsonSetValue(JsonValue* root, const char* path, JsonValue* value);
int JsonSetString(JsonValue* root, const char* path, const char* value);
int JsonSetNumber(JsonValue* root, const char* path, double value);
int JsonSetBool(JsonValue* root, const char* path, int value);
int JsonSetNull(JsonValue* root, const char* path);
int JsonSetObject(JsonValue* root, const char* path);
char* JsonSerialize(JsonValue* root);

// Disk utilities
char* loadFile(const char* filename);
void printValue(JsonValue* value);