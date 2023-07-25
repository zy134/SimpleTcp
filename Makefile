# ==============================================================================
# Set first character which introducing a recipe line.(Default it's a tab.)
# .RECIPEPREFIX = >

# ==============================================================================
# Set path variant
ROOT_PATH       := $(shell pwd)
BUILD_PATH      := $(ROOT_PATH)/build
LOG_PATH        := $(ROOT_PATH)/logs# -DDEFAULT_LOG_PATH=$(LOG_PATH)
INCLUDE_PATH    := $(ROOT_PATH)/include
SOURCE_PATH     := $(ROOT_PATH)/src
LIBRARY_PATH    := $(ROOT_PATH)/lib
LIBRARY_PATH    += $(BUILD_PATH)/lib

# ==============================================================================
# Set source files
BUILD_LOG           := $(BUILD_PATH)/log.txt
BASE_SOURCE_FILES   := $(wildcard $(SOURCE_PATH)/base/*.cpp)
NET_SOURCE_FILES    := $(wildcard $(SOURCE_PATH)/net/*.cpp)
TCP_SOURCE_FILES    := $(wildcard $(SOURCE_PATH)/tcp/*.cpp)
MAIN_SOURCE_FILES   := $(wildcard $(SOURCE_PATH)/main/*.cpp)

# ==============================================================================
# Set target objects
BASE_OBJS   := $(patsubst $(SOURCE_PATH)/base/%.cpp, $(BUILD_PATH)/base/%.o, $(BASE_SOURCE_FILES))
NET_OBJS    := $(patsubst $(SOURCE_PATH)/net/%.cpp, $(BUILD_PATH)/net/%.o, $(NET_SOURCE_FILES))
TCP_OBJS    := $(patsubst $(SOURCE_PATH)/tcp/%.cpp, $(BUILD_PATH)/tcp/%.o, $(TCP_SOURCE_FILES))
MAIN_OBJS   := $(patsubst $(SOURCE_PATH)/main/%.cpp, $(BUILD_PATH)/main/%, $(MAIN_SOURCE_FILES))

BASE_MODULE := $(BUILD_PATH)/lib/libMyUtils.a
NET_MODULE := $(BUILD_PATH)/lib/libMyNet.a
TCP_MODULE  := $(BUILD_PATH)/lib/libMyTcp.a

# ==============================================================================
# Set compile command and compile flags.
CC       := gcc
CXX      := clang++
CFLAGS   :=
CPPFLAGS := -std=c++20 -DDEFAULT_LOG_PATH=\"$(LOG_PATH)\" --include-directory=$(INCLUDE_PATH) -pthread -Wall -Werror

# compile flag for debug version
DEBUG_FLAGS := -Og -g -fno-omit-frame-pointer -DDEBUG_BUILD=1 -DDEFAULT_LOG_LEVEL=1
# compile flag for release version
RELEASE_FLAGS := -O2 -DRELEASE_BUILD=1 -DDEFAULT_LOG_LEVEL=2

# address sanitizer flags
ASAN_FLAGS := -fsanitize=address -fsanitize=leak -fsanitize=undefined
DEBUG_FLAGS += $(ASAN_FLAGS)

# default, build debug version.
ifeq ($(RELEASE_BUILD),true)
    CPPFLAGS += $(RELEASE_FLAGS)
    BUILD_VER := Release version
else
    CPPFLAGS += $(DEBUG_FLAGS)
    BUILD_VER := Debug version
endif

LINKFLAGS += $(TCP_MODULE) $(NET_MODULE) $(BASE_MODULE) -lfmt

# ==============================================================================
# Set make targets.

.PHONY: all config clean
all: $(MAIN_OBJS)

config: clean
	@ echo "=========================== start config ============================"
	mkdir $(BUILD_PATH)
	mkdir $(BUILD_PATH)/base
	mkdir $(BUILD_PATH)/net
	mkdir $(BUILD_PATH)/tcp
	mkdir $(BUILD_PATH)/main
	mkdir $(BUILD_PATH)/lib
	touch $(BUILD_LOG)
	chmod 0777 $(BUILD_LOG)
	@ echo "=========================== Source Files ============================"
	@ echo $(BASE_SOURCE_FILES)
	@ echo $(NET_SOURCE_FILES)
	@ echo $(TCP_SOURCE_FILES)
	@ echo $(MAIN_SOURCE_FILES)
	@ echo "=========================== Object files ============================"
	@ echo "base objects: $(notdir $(BASE_OBJS))"
	@ echo "network objects: $(notdir $(NET_OBJS))"
	@ echo "TCP objects: $(notdir $(TCP_OBJS))"
	@ echo "main objects: $(notdir $(MAIN_OBJS))"
	@ echo "build version: << $(BUILD_VER) >>"
	@ echo "========================== static library ==========================="
	@ echo "base: $(notdir $(BASE_MODULE))"
	@ echo "net: $(notdir $(NET_MODULE))"
	@ echo "tcp: $(notdir $(TCP_MODULE))"
	@ echo "============================ end config ============================="


$(BASE_OBJS): $(BASE_SOURCE_FILES)
	@ echo "[Compile] build target: $(notdir $@)"
	@ cd $(BUILD_PATH)/base && $(CXX) $(CPPFLAGS) -c $^

$(NET_OBJS): $(NET_SOURCE_FILES)
	@ echo "[Compile] build target: $(notdir $@)"
	@ cd $(BUILD_PATH)/net && $(CXX) $(CPPFLAGS) -c $^

$(TCP_OBJS): $(TCP_SOURCE_FILES)
	@ echo "[Compile] build target: $(notdir $@)"
	@ cd $(BUILD_PATH)/tcp && $(CXX) $(CPPFLAGS) -c $^

$(BASE_MODULE): $(BASE_OBJS)
	@ echo "[ar     ] archive static library: $(notdir $@)"
	@ cd $(BUILD_PATH)/base && ar rcs libMyUtils.a $(notdir $^) && cp libMyUtils.a $(BUILD_PATH)/lib

$(NET_MODULE): $(NET_OBJS)
	@ echo "[ar     ] archive static library: $(notdir $@)"
	@ cd $(BUILD_PATH)/net && ar rcs libMyNet.a $(notdir $^) && cp libMyNet.a $(BUILD_PATH)/lib

$(TCP_MODULE): $(TCP_OBJS)
	@ echo "[ar     ] archive static library: $(notdir $@)"
	@ cd $(BUILD_PATH)/tcp && ar rcs libMyTcp.a $(notdir $^) && cp libMyTcp.a $(BUILD_PATH)/lib

library: $(TCP_MODULE) $(NET_MODULE) $(BASE_MODULE)

$(MAIN_OBJS): $(MAIN_SOURCE_FILES) $(BASE_MODULE) $(NET_MODULE) $(TCP_MODULE)
	@ echo "[link   ] build main object: $(notdir $@)"
	@ cd $(BUILD_PATH)/main && \
		$(CXX) $(CPPFLAGS) $(SOURCE_PATH)/main/ChatServer.cpp $(LINKFLAGS) -o $(BUILD_PATH)/main/ChatServer
	@ cd $(BUILD_PATH)/main && \
		$(CXX) $(CPPFLAGS) $(SOURCE_PATH)/main/ChatClient.cpp $(LINKFLAGS) -o $(BUILD_PATH)/main/ChatClient
	@ cd $(BUILD_PATH)/main && \
		$(CXX) $(CPPFLAGS) $(SOURCE_PATH)/main/EchoServer.cpp $(LINKFLAGS) -o $(BUILD_PATH)/main/EchoServer
	@ cd $(BUILD_PATH)/main && \
		$(CXX) $(CPPFLAGS) $(SOURCE_PATH)/main/EchoClient.cpp $(LINKFLAGS) -o $(BUILD_PATH)/main/EchoClient

clean:
	@ echo "=============================== Clean ==============================="
	rm $(BUILD_PATH) -rf
	rm $(LOG_PATH)/* -rf
