BIN_NAME   := pamelo
BUILD_PATH := build
FINAL_PATH := $(BUILD_PATH)/$(BIN_NAME)

INCLUDE_PATH := include
SRC_PATH     := src
H_FILES      := $(shell find $(INCLUDE_PATH) -name '*.h')
C_FILES      := $(shell find $(SRC_PATH) -name '*.c')

CFLAGS ?=

$(FINAL_PATH): $(BUILD_PATH) $(H_FILES) $(C_FILES)
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) $(C_FILES) -o $(FINAL_PATH)

$(BUILD_PATH):
	mkdir -p $(BUILD_PATH)

.phony: clean
clean:
	rm -rf $(BUILD_PATH)

