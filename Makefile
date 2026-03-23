CXX = g++
OUT_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

.PHONY: all clean

all: $(OUT_DIR) $(OUT_DIR)/bvm $(OUT_DIR)/example $(OUT_DIR)/objdump

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(OUT_DIR)/bvm: $(SRC_DIR)/bvm.cpp | $(OUT_DIR)
	g++ -std=c++23 src/bvm.cpp -o $(OUT_DIR)/bvm -I$(INCLUDE_DIR)

$(OUT_DIR)/example: $(SRC_DIR)/example.cpp | $(OUT_DIR)
	g++ -std=c++23 src/example.cpp -o $(OUT_DIR)/example -I$(INCLUDE_DIR)

$(OUT_DIR)/objdump: $(SRC_DIR)/example.cpp | $(OUT_DIR)
	g++ -std=c++23 src/objdump.cpp -o $(OUT_DIR)/objdump -I$(INCLUDE_DIR)
clean:
	rm -rf $(OUT_DIR)
