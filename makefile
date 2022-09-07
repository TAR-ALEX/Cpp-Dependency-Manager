# BSD 3-Clause License

# Copyright (c) 2022, Alex Tarasov
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.

# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.

# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

CC=g++
CC_CPP=g++
CC_C=gcc

# CC=clang++
# CC_CPP=clang++
# CC_C=clang

CCFLAGS=-Wall -O3  -I"./vendor/include" -DBOOST_REGEX_STANDALONE=0

CPP_CCFlags=$(CCFLAGS) -std=c++17
C_CCFlags=$(CCFLAGS)

LDFLAGS=-O3 -std=c++11 -lstdc++fs -lssl -lcrypto -llzma -lz -lbz2 -lpthread #-lboost_regex

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
