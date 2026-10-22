NAME := obsidian

CXX ?= g++
CPPFLAGS += -Iinclude -MMD -MP
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
LDFLAGS ?=
LDLIBS ?=

SRC_DIR := src
OBJ_DIR := build/obj
BIN_DIR := build/bin
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

ifeq ($(OS),Windows_NT)
TARGET := $(BIN_DIR)/$(NAME).exe
JOBS ?= $(NUMBER_OF_PROCESSORS)
else
TARGET := $(BIN_DIR)/$(NAME)
JOBS ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
endif

ifneq ($(findstring sh,$(notdir $(SHELL))),)
define make-dir
	@mkdir -p "$1"
endef
define remove-dir
	@rm -rf "$1"
endef
else
define make-dir
	@if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
endef
define remove-dir
	@if exist "$(subst /,\,$1)" rmdir /S /Q "$(subst /,\,$1)"
endef
endif

ifeq ($(strip $(JOBS)),)
JOBS := 2
endif

.DEFAULT_GOAL := all

.PHONY: all parallel clean re

all: $(TARGET)

# Starts a parallel build using every detected logical processor.
parallel:
	+$(MAKE) --jobs=$(JOBS) all

$(TARGET): $(OBJECTS)
	$(call make-dir,$(@D))
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(call make-dir,$(@D))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(call remove-dir,build)

re: clean
	+$(MAKE) --jobs=$(JOBS) all

-include $(DEPENDENCIES)
