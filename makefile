CC=g++
CC_CPP=g++
CC_C=gcc

CCFLAGS=-Wall -O3  -I"./vendor/include"

CPP_CCFlags=$(CCFLAGS) -std=c++17
C_CCFlags=$(CCFLAGS)

LDFLAGS=-O3 -std=c++11 -lstdc++fs -lssl -lcrypto -llzma -lz -lbz2

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SOURCES := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c)
SOURCES += $(shell find ./vendor/src -name *.cpp -or -name *.c)

OBJECTS := $(SOURCES:%=$(BUILD_DIR)/%.o)
DEBUG_OBJECTS := $(SOURCES:%=$(BUILD_DIR)/%.d.o)

TARGET=git-vendor

# all: $(TARGET)
all: release

gprof:
	gprof $(TARGET)_DEBUG gmon.out  >output.txt

release: $(TARGET)
#	./$(TARGET)

debug: $(TARGET)_DEBUG
# 	gdb $(TARGET)_DEBUG
#	./$(TARGET)_DEBUG

$(TARGET)_DEBUG: $(DEBUG_OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) -ggdb -pg

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CC_CPP) $(CPP_CCFlags) -c $< -o $@
	
$(BUILD_DIR)/%.cpp.d.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CC_CPP) $(CPP_CCFlags) -c $< -o $@ -ggdb -pg

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC_C) $(C_CCFlags) -c $< -o $@
	
$(BUILD_DIR)/%.c.d.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC_C) $(C_CCFlags) -c $< -o $@ -ggdb -pg

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET) $(TARGET)_DEBUG
	
MKDIR_P ?= mkdir -p
