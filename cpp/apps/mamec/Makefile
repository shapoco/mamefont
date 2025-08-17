.PHONY: all build clean

REPO_DIR := $(shell cd ../../.. ; pwd)

BUILD_DIR := build
BIN_DIR := $(REPO_DIR)/bin

APP_NAME := mamec
APP_INC_DIR := include
APP_SRC_DIR := src
APP_HPP_LIST := $(wildcard $(APP_INC_DIR)/mamec/*.hpp)
APP_CPP_LIST := $(wildcard $(APP_SRC_DIR)/*.cpp)
APP_OBJ_LIST := $(patsubst $(APP_SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(APP_CPP_LIST))

MAMEFONT_DIR := ../..
MAMEFONT_INC_DIR := $(MAMEFONT_DIR)/include
MAMEFONT_SRC_DIR := $(MAMEFONT_DIR)/src
MAMEFONT_BUILD_DIR := $(BUILD_DIR)/mamefont
MAMEFONT_HPP_LIST := $(wildcard $(MAMEFONT_INC_DIR)/mamefont/*.hpp)
MAMEFONT_CPP_LIST := $(wildcard $(MAMEFONT_SRC_DIR)/*.cpp)
MAMEFONT_OBJ_LIST := $(patsubst $(MAMEFONT_SRC_DIR)/%.cpp,$(MAMEFONT_BUILD_DIR)/%.o,$(MAMEFONT_CPP_LIST))

STB_INC_DIR := $(REPO_DIR)/submodules/stb
JSON_INC_DIR := $(REPO_DIR)/submodules/json/include

BIN := $(BIN_DIR)/$(APP_NAME)

EXTRA_DEPENDENCIES := \
	Makefile

CXX := g++
CXXFLAGS := \
	-std=c++20 \
	-O2 \
	-DMAMEFONT_EXCEPTIONS \
	-DMAMEFONT_DEBUG \
	-I$(APP_INC_DIR) \
	-I$(MAMEFONT_INC_DIR) \
	-I$(STB_INC_DIR) \
	-I$(JSON_INC_DIR)
LDFLAGS := -lm

all: build

build: $(BIN)

$(BIN): $(APP_OBJ_LIST) $(MAMEFONT_OBJ_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $(APP_OBJ_LIST) $(MAMEFONT_OBJ_LIST) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(APP_SRC_DIR)/%.cpp $(APP_HPP_LIST) $(MAMEFONT_HPP_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAMEFONT_BUILD_DIR)/%.o: $(MAMEFONT_SRC_DIR)/%.cpp $(MAMEFONT_HPP_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)