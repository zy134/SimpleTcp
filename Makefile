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
BUILD_LOG               := $(BUILD_PATH)/log.txt
BASE_SOURCE_FILES       := $(wildcard $(SOURCE_PATH)/base/*.cpp)
NET_SOURCE_FILES        := $(wildcard $(SOURCE_PATH)/net/*.cpp)
TCP_SOURCE_FILES        := $(wildcard $(SOURCE_PATH)/tcp/*.cpp)
EXAMPLE_SOURCE_FILES    := $(wildcard $(SOURCE_PATH)/example/*.cpp)
ALL_SOURCE_FILE         := $(shell find . -name "*.cpp")
ALL_HEADER_FILE         := $(shell find . -name "*.h")

# ==============================================================================
# Set target objects
BASE_OBJS   := $(patsubst $(SOURCE_PATH)/base/%.cpp, $(BUILD_PATH)/base/%.o, $(BASE_SOURCE_FILES))
NET_OBJS    := $(patsubst $(SOURCE_PATH)/net/%.cpp, $(BUILD_PATH)/net/%.o, $(NET_SOURCE_FILES))
TCP_OBJS    := $(patsubst $(SOURCE_PATH)/tcp/%.cpp, $(BUILD_PATH)/tcp/%.o, $(TCP_SOURCE_FILES))
EXAMPLE_OBJS:= $(patsubst $(SOURCE_PATH)/example/%.cpp, $(BUILD_PATH)/example/%, $(EXAMPLE_SOURCE_FILES))

TCP_MODULE  := $(BUILD_PATH)/lib/libMyTcp.a

# ==============================================================================
# Set compile command and compile flags.
CC       := gcc
CXX      := g++
CFLAGS   :=
CPPFLAGS :=

ifeq ($(CXX),clang++)
    CPPFLAGS := -std=c++20 -DDEFAULT_LOG_PATH=\"$(LOG_PATH)\" --include-directory=$(INCLUDE_PATH) \
                -pthread -Wall -Wno-c++2b-extensions -Werror \
                -Wextra -Wconversion -Wshadow
else ifeq ($(CXX),g++)
    CPPFLAGS := -std=c++20 -DDEFAULT_LOG_PATH=\"$(LOG_PATH)\" --include-directory=$(INCLUDE_PATH) \
                -pthread -Wall -Werror \
                -Wextra -Wconversion -Wshadow
endif

# compile flag for debug version
DEBUG_FLAGS := -Og -g -fno-omit-frame-pointer -DDEBUG_BUILD=1 -DDEFAULT_LOG_LEVEL=1
# g++ need -rdynamic option to generate invocation infomation in stack frame.
ifeq ($(CXX),g++)
    DEBUG_FLAGS += -rdynamic
endif
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

LINKFLAGS += $(TCP_MODULE) -lfmt

# ==============================================================================
# Set make targets.

.PHONY: all
all: $(BASE_OBJS) $(NET_OBJS) $(TCP_OBJS) $(EXAMPLE_OBJS)

START_TIME := $(shell cat /proc/uptime | awk -F "." '{print $$1}')

.PHONY: config
config: clean
	@ echo "=========================== start config ============================"
	mkdir $(BUILD_PATH)
	mkdir $(BUILD_PATH)/base
	mkdir $(BUILD_PATH)/net
	mkdir $(BUILD_PATH)/tcp
	mkdir $(BUILD_PATH)/example
	mkdir $(BUILD_PATH)/lib
	touch $(BUILD_LOG)
	chmod 0777 $(BUILD_LOG)
	@ echo "=========================== Source Files ============================"
	@ echo $(BASE_SOURCE_FILES)
	@ echo $(NET_SOURCE_FILES)
	@ echo $(TCP_SOURCE_FILES)
	@ echo $(EXAMPLE_SOURCE_FILES)
	@ echo "=========================== Object files ============================"
	@ echo "base objects: $(notdir $(BASE_OBJS))"
	@ echo "network objects: $(notdir $(NET_OBJS))"
	@ echo "TCP objects: $(notdir $(TCP_OBJS))"
	@ echo "example objects: $(notdir $(EXAMPLE_OBJS))"
	@ echo "build version: << $(BUILD_VER) >>"
	@ echo "========================== static library ==========================="
	@ echo "tcp: $(notdir $(TCP_MODULE))"
	@ echo "============================ end config ============================="


$(BASE_OBJS): $(BASE_SOURCE_FILES)
	@ current_time=`date +"%x %X:%3N"`;\
		echo "[$${current_time}][Compile] build target: $(notdir $@)";\
		cd $(BUILD_PATH)/base;\
		$(CXX) $(CPPFLAGS) -c $^

$(NET_OBJS): $(NET_SOURCE_FILES)
	@ current_time=`date +"%x %X:%3N"`;\
		echo "[$${current_time}][Compile] build target: $(notdir $@)";\
		cd $(BUILD_PATH)/net;\
		$(CXX) $(CPPFLAGS) -c $^

$(TCP_OBJS): $(TCP_SOURCE_FILES)
	@ current_time=`date +"%x %X:%3N"`;\
		echo "[$${current_time}][Compile] build target: $(notdir $@)";\
		cd $(BUILD_PATH)/tcp;\
		$(CXX) $(CPPFLAGS) -c $^

$(TCP_MODULE): $(TCP_OBJS) $(NET_OBJS) $(BASE_OBJS)
	@ current_time=`date +"%x %X:%3N"`;\
		echo "[$${current_time}][Compile] build target: $(notdir $@)";\
		cd $(BUILD_PATH);\
		ar rcs libMyTcp.a $(TCP_OBJS) $(NET_OBJS) $(BASE_OBJS) && mv libMyTcp.a $(BUILD_PATH)/lib
	@ current_time=`date +"%x %X:%3N"`;\
		END_TIME=`cat /proc/uptime | awk -F "." '{print $$1}'`; \
		time_interval=`expr $${END_TIME} - $(START_TIME)`; \
		runtime=`date -u -d @$${time_interval} +%Hh:%Mm:%Ss`; \
		echo "[$${current_time}][runtime] success build library, total run time: $${runtime}"

library: $(TCP_MODULE)

$(EXAMPLE_OBJS): $(EXAMPLE_SOURCE_FILES) $(TCP_MODULE)
	@ current_time=`date +"%x %X:%3N"`;\
		echo "[$${current_time}][link   ] build example object: $(notdir $@)";\
		cd $(BUILD_PATH)/example; \
		$(CXX) $(CPPFLAGS) $(SOURCE_PATH)/example/$(notdir $@).cpp $(LINKFLAGS) -o $(BUILD_PATH)/example/$(notdir $@); \
		current_time=`date +"%x %X:%3N"`;\
		END_TIME=`cat /proc/uptime | awk -F "." '{print $$1}'`; \
		time_interval=`expr $${END_TIME} - $(START_TIME)`; \
		runtime=`date -u -d @$${time_interval} +%Hh:%Mm:%Ss`; \
		echo "[$${current_time}][runtime] success build example, total run time: $${runtime}"

.PHONY: clean
clean:
	@ echo "=============================== Clean ==============================="
	rm $(BUILD_PATH) -rf
	rm $(LOG_PATH)/* -rf
