CC = gcc
DIR ?= .
C_SOURCES := $(wildcard $(DIR)/*.c)
OBJECTS := $(C_SOURCES:.c=.o)
TARGET := program

all: $(TARGET)

$(TARGET): $(OBJECTS)
    $(CC) -o $@ $(OBJECTS)

%.o: %.c
    $(CC) -c $< -o $@

clean:
    rm -f $(DIR)/*.o $(TARGET)

.PHONY: all clean