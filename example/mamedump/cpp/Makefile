.PHONY: all build run clean

APP_NAME := mamedump

APP_SRC_DIR := src
APP_INC_DIR := include
APP_BUILD_DIR := build
APP_TARGET_HPP := $(APP_INC_DIR)/target_font.hpp
APP_CPP_LIST := $(wildcard $(APP_SRC_DIR)/*.cpp)
APP_HPP_LIST := $(wildcard $(APP_INC_DIR)/*.hpp)
APP_OBJ_LIST := $(patsubst $(APP_SRC_DIR)/%.cpp,$(APP_BUILD_DIR)/%.o,$(APP_CPP_LIST))

# Regenerate this unconditionally
.PHONY: $(APP_TARGET_HPP)

MAMEFONT_DIR := ../../../cpp
MAMEFONT_SRC_DIR := $(MAMEFONT_DIR)/src
MAMEFONT_INC_DIR := $(MAMEFONT_DIR)/include
MAMEFONT_BUILD_DIR := $(APP_BUILD_DIR)/mamefont
MAMEFONT_CPP_LIST := $(wildcard $(MAMEFONT_SRC_DIR)/*.cpp)
MAMEFONT_HPP_LIST := $(wildcard $(MAMEFONT_INC_DIR)/mamefont/*.hpp)
MAMEFONT_OBJ_LIST := $(patsubst $(MAMEFONT_SRC_DIR)/%.cpp,$(MAMEFONT_BUILD_DIR)/%.o,$(MAMEFONT_CPP_LIST))

FONT_NAME := MameSansP_s48c40w08
FONT_INC_DIR := ../../../../shapofont/mamefont/cpp/HL/include/shapofont
FONT_SRC_DIR := ../../../../shapofont/mamefont/cpp/HL/src
FONT_BUILD_DIR := $(APP_BUILD_DIR)/font
FONT_HPP := $(FONT_INC_DIR)/$(FONT_NAME).hpp
FONT_OBJ := $(FONT_BUILD_DIR)/$(FONT_NAME).o

EXTRA_DEPENDENCIES := \
	$(APP_HPP_LIST) \
	$(MAMEFONT_HPP_LIST) \
	$(FONT_HPP) \
	Makefile

BIN := $(APP_BUILD_DIR)/$(APP_NAME)

CXX := g++
CXXFLAGS := \
	-I$(APP_INC_DIR) \
	-I$(MAMEFONT_INC_DIR) \
	-I$(FONT_INC_DIR) \
	-DMAMEFONT_DEBUG \
	-DTARGET_FONT_NAME=$(FONT_NAME)

all: build

build: $(BIN)

$(BIN): $(APP_OBJ_LIST) $(MAMEFONT_OBJ_LIST) $(FONT_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(APP_BUILD_DIR)/%.o:  $(APP_SRC_DIR)/%.cpp $(APP_TARGET_HPP) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(APP_TARGET_HPP):
	@mkdir -p $(dir $@)
	echo "#include \"$(FONT_NAME).hpp\"" > $@

$(MAMEFONT_BUILD_DIR)/%.o: $(MAMEFONT_SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@	

$(FONT_BUILD_DIR)/%.o: $(FONT_SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(APP_BUILD_DIR)
	rm -f $(APP_TARGET_HPP)
