.PHONY: all build flash clean

APP_NAME := tiny402_ssd1306

REPO_DIR := ../../..
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
TOOLS_DIR := ../tools

MCU := attiny402
F_CPU := 20000000

CXX := $(AVR_GCC_PATH)/bin/avr-g++
OBJCOPY := $(AVR_GCC_PATH)/bin/avr-objcopy
AVR_SIZE := $(AVR_GCC_PATH)/bin/avr-size

HPP_LIST := $(wildcard $(INC_DIR)/*.hpp)
CPP_LIST := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_LIST := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_LIST)) 

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
	-DMAMEFONT_VERTICAL_FRAGMENT_ONLY \
	-DMAMEFONT_8BIT_ADDR \
	-I$(INC_DIR) \
	-I$(MAMEFONT_INC_DIR)

LDFLAGS := \
	-Os \
	-mmcu=$(MCU)

EXTRA_DEPENDENCIES := \
	$(HPP_LIST) \
	$(MAMEFONT_HPP_LIST) \
	Makefile

all: build

build: $(HEX)

$(HEX): $(ELF) $(EXTRA_DEPENDENCIES)
	$(OBJCOPY) -j .text -j .data -j .rodata -O ihex $< $@

$(ELF): $(OBJ_LIST) $(MAMEFONT_OBJ_LIST) $(EXTRA_DEPENDENCIES)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ_LIST) $(MAMEFONT_OBJ_LIST) $(FONT_OBJ_LIST)
	$(AVR_SIZE) $(ELF)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAMEFONT_BUILD_DIR)/%.o: $(MAMEFONT_SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
	@mkdir -p $(MAMEFONT_BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# $(FONT_BUILD_DIR)/%.o: $(FONT_SRC_DIR)/%.cpp $(EXTRA_DEPENDENCIES)
# 	@mkdir -p $(FONT_BUILD_DIR)
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

include $(TOOLS_DIR)/avrdude_wsl/tools.mk

clean:
	rm -rf $(BUILD_DIR)
