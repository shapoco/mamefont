.PHONY: all build font flash clean

APP_NAME := tiny85_ili9488_big_char

REPO_DIR := ../../..
BUILD_DIR := build
TOOLS_DIR := ../tools

MCU := attiny85
F_CPU := 16000000

CXX := $(AVR_GCC_PATH)/bin/avr-g++
OBJCOPY := $(AVR_GCC_PATH)/bin/avr-objcopy
AVR_SIZE := $(AVR_GCC_PATH)/bin/avr-size
MAMEC := $(REPO_DIR)/bin/mamec

FONT_BMP_DIR := bmp/font
FONT_INC_DIR := include/font
FONT_BMP_LIST := $(wildcard $(FONT_BMP_DIR)/*/design.png)
FONT_HPP_LIST := $(patsubst $(FONT_BMP_DIR)/%/design.png,$(FONT_INC_DIR)/%.hpp,$(FONT_BMP_LIST))

APP_SRC_DIR := src
APP_BMP_DIR := bmp
APP_INC_DIR := include
APP_HPP_LIST := $(wildcard $(APP_INC_DIR)/*.hpp) $(wildcard $(APP_INC_DIR)/tiny85/*.hpp) $(FONT_HPP_LIST)
APP_CPP_LIST := $(wildcard $(APP_SRC_DIR)/*.cpp)
APP_OBJ_LIST := $(patsubst $(APP_SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(APP_CPP_LIST)) 

MAMEFONT_INC_DIR := $(REPO_DIR)/cpp/include
MAMEFONT_SRC_DIR := $(REPO_DIR)/cpp/src
MAMEFONT_BUILD_DIR := $(BUILD_DIR)/mamefont
MAMEFONT_HPP_LIST := $(wildcard $(MAMEFONT_INC_DIR)/mamefont/*.hpp)
MAMEFONT_CPP_LIST := $(wildcard $(MAMEFONT_SRC_DIR)/*.cpp)
MAMEFONT_OBJ_LIST := $(patsubst $(MAMEFONT_SRC_DIR)/%.cpp,$(MAMEFONT_BUILD_DIR)/%.o,$(MAMEFONT_CPP_LIST))

ELF := $(BUILD_DIR)/$(APP_NAME).elf
HEX := $(BUILD_DIR)/$(APP_NAME).hex

CXXFLAGS := \
	-Os \
	-mmcu=$(MCU) \
	-DF_CPU=$(F_CPU) \
	-DMAMEFONT_USE_PROGMEM \
	-DMAMEFONT_NEAR1ST_ONLY \
	-DMAMEFONT_1BPP_ONLY \
	-DMAMEFONT_PROPORTIONAL_ONLY \
	-DMAMEFONT_HORI_FRAG_ONLY \
	-DMAMEFONT_NO_EXT_HEADER \
	-I$(APP_INC_DIR) \
	-I$(APP_INC_DIR)/font \
	-I$(MAMEFONT_INC_DIR)


LDFLAGS := \
	-Os \
	-mmcu=$(MCU)

EXTRA_DEPENDENCIES := \
	Makefile \
	$(MAMEC)

all: build

build: $(HEX)

font: $(FONT_HPP_LIST)

$(HEX): $(ELF) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(OBJCOPY) -j .text -j .data -j .rodata -O ihex $< $@

$(ELF): $(APP_OBJ_LIST) $(FONT_OBJ_LIST) $(MAMEFONT_OBJ_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $(APP_OBJ_LIST) $(FONT_OBJ_LIST) $(MAMEFONT_OBJ_LIST)
	$(AVR_SIZE) $(ELF)

$(BUILD_DIR)/%.o: $(APP_SRC_DIR)/%.cpp $(APP_HPP_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAMEFONT_BUILD_DIR)/%.o: $(MAMEFONT_SRC_DIR)/%.cpp $(MAMEFONT_HPP_LIST) $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(FONT_INC_DIR)/%.hpp: $(FONT_BMP_DIR)/%/design.png $(FONT_BMP_DIR)/%/design.json $(EXTRA_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(MAMEC) \
		-e HL \
		-i $< \
		-o $@

include $(TOOLS_DIR)/avrdude_wsl/tools.mk

clean:
	rm -rf $(BUILD_DIR)
