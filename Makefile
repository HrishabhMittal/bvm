CXX = g++
OUT_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

.PHONY: all clean

all: $(OUT_DIR) $(OUT_DIR)/bvm $(OUT_DIR)/objdump

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(OUT_DIR)/bvm: include $(SRC_DIR)/bvm.cpp | $(OUT_DIR)
	g++ -std=c++23 src/bvm.cpp -o $(OUT_DIR)/bvm -I$(INCLUDE_DIR)

$(OUT_DIR)/objdump: include $(SRC_DIR)/objdump.cpp | $(OUT_DIR)
	g++ -std=c++23 src/objdump.cpp -o $(OUT_DIR)/objdump -I$(INCLUDE_DIR)

clean:
	rm -rf $(OUT_DIR)
