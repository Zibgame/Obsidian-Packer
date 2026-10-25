NAME := obsidian

CXX ?= g++
CC ?= gcc
CPPFLAGS += -Iinclude -Isrc/third_party/lzma -MMD -MP
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
CFLAGS ?= -O2 -Wall -D_7ZIP_ST -DZ7_ST
LDFLAGS ?= -s
LDLIBS ?=

SRC_DIR := src
STUB_DIR := $(SRC_DIR)/stub
CLI_DIR := $(SRC_DIR)/cli
PACKER_DIR := $(SRC_DIR)/packer
LOADER_DIR := $(SRC_DIR)/loader
LZMA_DIR := $(SRC_DIR)/third_party/lzma
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := .

MAIN_SOURCES := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(CLI_DIR)/*.cpp) $(wildcard $(PACKER_DIR)/*.cpp) $(wildcard $(LOADER_DIR)/*.cpp)
STUB_SOURCES := $(wildcard $(STUB_DIR)/*.cpp)
COMMON_SOURCES := $(wildcard $(SRC_DIR)/file/*.cpp) $(wildcard $(SRC_DIR)/compression/*.cpp) $(wildcard $(SRC_DIR)/crypto/*.cpp) $(wildcard $(SRC_DIR)/pe/*.cpp)
LZMA_SOURCES := $(wildcard $(LZMA_DIR)/*.c)
MAIN_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(MAIN_SOURCES))
STUB_OBJECTS := $(patsubst $(STUB_DIR)/%.cpp,$(OBJ_DIR)/stub/%.o,$(STUB_SOURCES))
COMMON_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(COMMON_SOURCES))
LZMA_OBJECTS := $(patsubst $(LZMA_DIR)/%.c,$(OBJ_DIR)/lzma/%.o,$(LZMA_SOURCES))
DEPENDENCIES := $(MAIN_OBJECTS:.o=.d) $(STUB_OBJECTS:.o=.d) $(COMMON_OBJECTS:.o=.d) $(LZMA_OBJECTS:.o=.d)

ifeq ($(OS),Windows_NT)
HOST_OS := windows
else
HOST_OS := linux
endif

TARGET_OS ?= $(HOST_OS)

ifeq ($(TARGET_OS),windows)
TARGET := $(BIN_DIR)/$(NAME).exe
STUB_TARGET := $(BUILD_DIR)/$(NAME)-stub.exe
ALL_TARGETS := $(TARGET) $(STUB_TARGET)
PACKED_TARGET := output.exe
EXTRACTED_TARGET := extracted.bin
LDLIBS += -lbcrypt
WINDRES ?= windres
WINDOWS_ICON := assets/windows/obsidian.ico
WINDOWS_RESOURCE_SOURCE := assets/windows/obsidian.rc
WINDOWS_RESOURCE_OBJECT := $(OBJ_DIR)/windows/obsidian-resource.o
else
TARGET := $(BIN_DIR)/$(NAME)
STUB_TARGET := $(BUILD_DIR)/$(NAME)-stub
ALL_TARGETS := $(TARGET)
PACKED_TARGET := output.exe
EXTRACTED_TARGET := extracted.bin
WINDOWS_RESOURCE_OBJECT :=
endif

ifneq ($(findstring sh,$(notdir $(SHELL))),)
WINDRES_CPP ?= $(shell command -v $(CC)) -E -xc-header -DRC_INVOKED
define make-dir
	@mkdir -p "$1"
endef
define remove-dir
	@rm -rf "$1"
endef
define remove-file
	@rm -f "$1"
endef
else
WINDRES_CPP ?= $(CC) -E -xc-header -DRC_INVOKED
define make-dir
	@if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
endef
define remove-dir
	@if exist "$(subst /,\,$1)" rmdir /S /Q "$(subst /,\,$1)"
endef
define remove-file
	@if exist "$(subst /,\,$1)" del /Q "$(subst /,\,$1)"
endef
endif

ifeq ($(strip $(JOBS)),)
JOBS := 2
endif

ifeq ($(OS),Windows_NT)
JOBS ?= $(NUMBER_OF_PROCESSORS)
else
JOBS ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
endif

.DEFAULT_GOAL := all

.PHONY: all parallel clean re examples windows linux stub-mingw

all: $(ALL_TARGETS)

windows:
	+$(MAKE) --jobs=$(JOBS) TARGET_OS=windows all

linux:
	+$(MAKE) --jobs=$(JOBS) TARGET_OS=linux all

# Starts a parallel build using every detected logical processor.
parallel:
	+$(MAKE) --jobs=$(JOBS) all

$(TARGET): $(MAIN_OBJECTS) $(COMMON_OBJECTS) $(LZMA_OBJECTS) $(WINDOWS_RESOURCE_OBJECT)
	$(CXX) $(LDFLAGS) $(MAIN_OBJECTS) $(COMMON_OBJECTS) $(LZMA_OBJECTS) $(WINDOWS_RESOURCE_OBJECT) $(LDLIBS) -o $@

$(STUB_TARGET): $(STUB_OBJECTS) $(COMMON_OBJECTS) $(LZMA_OBJECTS)
	$(call make-dir,$(@D))
	$(CXX) $(LDFLAGS) $(STUB_OBJECTS) $(COMMON_OBJECTS) $(LZMA_OBJECTS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(call make-dir,$(@D))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/stub/%.o: $(STUB_DIR)/%.cpp
	$(call make-dir,$(@D))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/lzma/%.o: $(LZMA_DIR)/%.c
	$(call make-dir,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

ifeq ($(TARGET_OS),windows)
$(WINDOWS_RESOURCE_OBJECT): $(WINDOWS_RESOURCE_SOURCE) $(WINDOWS_ICON)
	$(call make-dir,$(@D))
	$(WINDRES) --preprocessor="$(WINDRES_CPP)" -Iassets/windows $< -O coff -o $@
endif

# Cross-builds the Windows stub from Linux (needs mingw-w64).
ifeq ($(TARGET_OS),linux)
MINGW_CXX ?= x86_64-w64-mingw32-g++
MINGW_CC ?= x86_64-w64-mingw32-gcc
MINGW_OBJ_DIR := $(BUILD_DIR)/mingw
MINGW_STUB_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(MINGW_OBJ_DIR)/%.o,$(STUB_SOURCES) $(COMMON_SOURCES)) $(patsubst $(LZMA_DIR)/%.c,$(MINGW_OBJ_DIR)/lzma/%.o,$(LZMA_SOURCES))

$(MINGW_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(call make-dir,$(@D))
	$(MINGW_CXX) $(CPPFLAGS) -std=c++17 -O2 -Wall -c $< -o $@

$(MINGW_OBJ_DIR)/lzma/%.o: $(LZMA_DIR)/%.c
	$(call make-dir,$(@D))
	$(MINGW_CC) $(CPPFLAGS) -O2 -Wall -D_7ZIP_ST -DZ7_ST -c $< -o $@

stub-mingw: $(MINGW_STUB_OBJECTS)
	$(call make-dir,$(BUILD_DIR))
	$(MINGW_CXX) -s $(MINGW_STUB_OBJECTS) -lbcrypt -o $(BUILD_DIR)/$(NAME)-stub.exe
endif

clean:
	$(call remove-dir,build)
	$(call remove-file,$(TARGET))
	$(call remove-file,$(STUB_TARGET))
	$(call remove-file,$(PACKED_TARGET))
	$(call remove-file,$(EXTRACTED_TARGET))

re: clean
	+$(MAKE) --jobs=$(JOBS) all

-include $(DEPENDENCIES)

EXAMPLE_DIR := examples
EXAMPLE_BINS := $(EXAMPLE_DIR)/hello.exe $(EXAMPLE_DIR)/tls_demo.exe $(EXAMPLE_DIR)/args_echo.exe

ifeq ($(TARGET_OS),windows)
examples: $(EXAMPLE_BINS)

$(EXAMPLE_DIR)/%.exe: $(EXAMPLE_DIR)/%.c $(STUB_TARGET)
	$(CXX) $(CXXFLAGS) $< -o $@
else
examples:
	@echo "examples are Windows PE fixtures; build them with 'make windows'"
endif

clean-examples:
	$(call remove-dir,$(EXAMPLE_DIR))
