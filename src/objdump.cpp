#include "bytecode.hpp"
#include "vm.hpp"
#include <bit>
#include <bvm.hpp>
#include <cctype>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

void printStringSafely(const std::string &s) {
    for (auto i : s) {
        if (std::isprint(i))
            std::cout << i;
        else
            std::cout << '.';
    }
}

int main(int argc, char **argv) {
    if (argc != 2)
        return 1;
    std::ifstream file(argv[1], std::ios::binary);
    auto v = bvm::load_bytecode(file);
    int64_t num = 0;
    std::cout << "HEADER SECTION: " << std::endl;
    printStringSafely(v.header);
    std::cout << std::endl << "CODE SECTION: " << std::endl;
    for (auto i : v.code) {
        std::cout << num << ": " << i.op;
        if (bvm::takes_operand(i.op)) {
            std::cout << ' ' << std::bit_cast<int64_t>(i.operands[0]);
        }
        std::cout << std::endl;
        num++;
    }
    std::cout << "DATA SECTION: " << std::endl;
    printStringSafely(v.data);
    std::cout << std::endl;
    file.close();
}
