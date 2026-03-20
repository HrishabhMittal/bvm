#include "vm.cpp"

int main() {
    std::vector<instruction> code = {
        {OPCODE::PUSH, {10}},
        {OPCODE::PUSH, {20}},
        {OPCODE::IADD, {}},
        {OPCODE::PRINT_I, {}},
        {OPCODE::HALT, {}}
    };
    VM vm(code);
    vm.exec();

    return 0;
}
