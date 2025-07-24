.PHONY: all build flash clean

APP_NAME := tiny402_ssd1306

REPO_DIR := ../../..
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
FONT_BUILD_DIR := $(BUILD_DIR)/font
TOOLS_DIR := ../tools

MAMEFONT_INC_DIR := $(REPO_DIR)/cpp/include

FONT_INC_DIR := ../../fonts/cpp/include
FONT_SRC_DIR := ../../fonts/cpp/src/shapofont

MCU := attiny402
F_CPU := 20000000

CXX := $(AVR_GCC_PATH)/bin/avr-g++
OBJCOPY := $(AVR_GCC_PATH)/bin/avr-objcopy
AVR_SIZE := $(AVR_GCC_PATH)/bin/avr-size

CPP_LIST := $(wildcard $(SRC_DIR)/*.cpp)
HPP_LIST := $(wildcard $(INC_DIR)/*.hpp)
OBJ_LIST := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_LIST)) 

MAMEFONT_HPP_LIST := $(wildcard $(MAMEFONT_INC_DIR)/mamefont/*.hpp)

FONT_CPP_LIST := 
FONT_CPP_LIST += $(FONT_SRC_DIR)/ShapoSansP_s11c09w2a1.cpp
FONT_CPP_LIST += $(FONT_SRC_DIR)/ShapoSansDigitP_s16c14w2.cpp

FONT_HPP_LIST := $(wildcard $(FONT_INC_DIR)/shapofont/*.hpp)
FONT_OBJ_LIST := $(patsubst $(FONT_SRC_DIR)/%.cpp,$(FONT_BUILD_DIR)/%.o,$(FONT_CPP_LIST))

ELF := $(BUILD_DIR)/$(APP_NAME).elf
HEX := $(BUILD_DIR)/$(APP_NAME).hex

CXXFLAGS := \
	-Os \
	-mmcu=$(MCU) \
	-DF_CPU=$(F_CPU) \
	-DMAMEFONT_HORIZONTAL_SCAN_ONLY \
	-DMAMEFONT_8BIT_ADDR \
	-I$(INC_DIR) \
	-I$(MAMEFONT_INC_DIR) \
	-I$(FONT_INC_DIR)

LDFLAGS := \
	-Os \
	-mmcu=$(MCU)

EXTRA_DEPENDENCIES := \
	$(HPP_LIST) \
	$(MAMEFONT_HPP_LIST) \
	$(FONT_HPP_LIST) \
	Makefile

all: build

build: $(HEX)

$(HEX): $(ELF) $(EXTRA_DEPENDENCIES)
	$(OBJCOPY) -j .text -j .data -j .rodata -O ihex $< $@

$(ELF): $(OBJ_LIST) $(FONT_OBJ_LIST) $(EXTRA_DEPENDENCIES)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ_LIST) $(FONT_OBJ_LIST)
	$(AVR_SIZE) $(ELF)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(FONT_BUILD_DIR)/%.o: $(FONT_SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
	@mkdir -p $(FONT_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

include $(TOOLS_DIR)/avrdude_wsl/tools.mk

clean:
	rm -rf $(BUILD_DIR)
