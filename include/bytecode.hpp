#pragma once
#include "opcode.hpp"
#include "vm.hpp"
#include <cstdint>
#include <fstream>
namespace bvm {
inline std::vector<instruction> load_bytecode(std::ifstream &in) {
    uint8_t byte;
    uint64_t size;
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    std::vector<instruction> out(size);
    for (uint64_t i = 0; i < size; i++) {
        in.read(reinterpret_cast<char *>(&byte), sizeof(byte));
        uint64_t operand = 0;
        if (takes_operand(static_cast<OPCODE>(byte))) {
            in.read(reinterpret_cast<char *>(&operand), sizeof(operand));
        }
        out[i] = {static_cast<OPCODE>(byte), {operand}};
    }
    return out;
}
inline void dump_bytecode(const std::vector<instruction> &ins, std::ofstream &out) {
    uint64_t size = ins.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (auto &i : ins) {
        out.write(reinterpret_cast<const char *>(&i.op), sizeof(i.op));
        if (takes_operand(i.op)) {
            out.write(reinterpret_cast<const char *>(i.operands), sizeof(i.operands));
        }
    }
}
} // namespace bvm
