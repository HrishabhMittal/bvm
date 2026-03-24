#include "bytecode.hpp"
#include "vm.hpp"
#include <bit>
#include <bvm.hpp>
#include <fstream>
#include <ios>
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 2)
        return 1;
    std::ifstream file(argv[1], std::ios::binary);
    auto v = bvm::load_bytecode(file);
    int64_t num = 0;
    for (auto i : v) {
        std::cout << num << ": " << i.op;
        if (bvm::takes_operand(i.op)) {
            std::cout << ' ' << std::bit_cast<int64_t>(i.operands[0]);
        }
        std::cout << std::endl;
        num++;
    }
    file.close();
}
