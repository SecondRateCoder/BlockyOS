#pragma once

#include "efi.h"
#include "efilib.h"
#include "kernel/libcrt/def.h"
#include "src/Boot/UEFI/tools/tools.h"

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
void JsonFree(JsonValue* root);

// Path resolution and management suite
JsonValue* JsongetValue(JsonValue* root, const char* path);

// Disk utilities