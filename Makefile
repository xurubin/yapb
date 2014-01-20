SRC_DIR   := source
BUILD_DIR := build

CC := g++
LD := g++

CFLAGS := -DPLATFORM_LINUX32

vpath $.cpp $(SRC_DIR)

SRC       := $(wildcard $(SRC_DIR)/*.cpp)
OBJ       := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))

all: $(BUILD_DIR)/yapb.so

install: all
	cp $(BUILD_DIR)/yapb.so ~/hlds_5447/cstrike/addons/yapb/dlls/yapb_i686.so

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $< -Iinclude/ -c $(CFLAGS) -o $@

$(BUILD_DIR)/yapb.so: $(OBJ)
	$(LD) $^ -shared -o $@

.phony: all
