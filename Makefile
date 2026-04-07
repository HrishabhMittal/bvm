CXX = g++
CXXFLAGS = -std=c++23 -Ofast -march=native
OUT_DIR = build
SRC_DIR = src
STD_DIR = stdlib
INCLUDE_DIR = include

.PHONY: all clean

all: $(OUT_DIR) $(OUT_DIR)/bvm $(OUT_DIR)/objdump $(OUT_DIR)/stdprint

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(OUT_DIR)/bvm: include $(SRC_DIR)/bvm.cpp | $(OUT_DIR)
	g++ $(CXXFLAGS) $(SRC_DIR)/bvm.cpp -o $(OUT_DIR)/bvm -I$(INCLUDE_DIR)

$(OUT_DIR)/objdump: include $(SRC_DIR)/objdump.cpp | $(OUT_DIR)
	g++ $(CXXFLAGS) $(SRC_DIR)/objdump.cpp -o $(OUT_DIR)/objdump -I$(INCLUDE_DIR)

$(OUT_DIR)/stdprint: include $(STD_DIR)/print.cpp | $(OUT_DIR)
	g++ $(CXXFLAGS) $(STD_DIR)/print.cpp -o $(OUT_DIR)/stdprint -I$(INCLUDE_DIR)

clean:
	rm -rf $(OUT_DIR)
