#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/tools/tools.h"

#define MiniJsonFPrefix static inline

typedef enum{
	JTYPE_STRING,
	JTYPE_NUMBER,
	JTYPE_BOOL,
	JTYPE_NULL,
	JTYPE_OBJECT
}JsonType;

// Forward declaration
struct JsonValue;

typedef struct JsonPair{
	char* key;
	struct JsonValue* value;
}JsonPair;

// JSON value structure
typedef struct JsonValue{
	JsonType type;
	union{
		char* stringValue;
		double numberValue;
		int boolValue;
		struct{
			JsonPair* pairs;
			int count;
		}objectValue;
	};
}JsonValue;

void printValue(JsonValue* value);
JsonValue* readValue(const char** text);
JsonValue* getValue(JsonValue* root, const char* path);