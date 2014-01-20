SRC_DIR   := source
BUILD_DIR := build

CC := g++
LD := g++

vpath $.cpp $(SRC_DIR)

SRC       := $(wildcard $(SRC_DIR)/*.cpp)
OBJ       := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))

all: $(BUILD_DIR)/yapb.so

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $< -fPIC -Iinclude/ -o $@

$(BUILD_DIR)/yapb.so: $(OBJ)
	$(LD) $^ -shared -o $@

.phony: all
