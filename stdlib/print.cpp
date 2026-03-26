#include "bytecode.hpp"
#include <fstream>
#include <iostream>
#include <string>

using namespace bvm;

int main(int argc, char **argv) {
    if (argc != 2)
        return 1;
    std::string prefix = argv[1];
    program stdlib;
    stdlib.header = "BOLT_STDLIB_V1";
    auto add_print_func = [&](const std::string &name, OPCODE print_op) {
        std::string extended_name = prefix + '.' + name;
        stdlib.exported_functions[extended_name] = stdlib.code.size();
        stdlib.code.push_back({print_op, {0}});
        stdlib.code.push_back({OPCODE::RET, {0}});
    };

    add_print_func("printi32", OPCODE::PRINT_I32);
    add_print_func("printi64", OPCODE::PRINT_I64);
    add_print_func("printf32", OPCODE::PRINT_F32);
    add_print_func("printf64", OPCODE::PRINT_F64);
    add_print_func("printgstring", OPCODE::PRINT_STRING);

    std::ofstream out("print", std::ios::binary);
    dump_bytecode(stdlib, out);
    out.close();
    return 0;
}
