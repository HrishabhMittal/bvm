#pragma once
#include "opcode.hpp"
#include "vm.hpp"
#include <cstdint>
#include <fstream>
#include <string>
namespace bvm {
inline program load_bytecode(std::ifstream &in) {
    program out;
    uint8_t byte;
    uint64_t size;
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    out.header.resize(size);
    for (uint64_t i = 0; i < size; i++) {
        in.read(reinterpret_cast<char *>(&byte), sizeof(byte));
        out.header[i] = byte;
    }
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    out.code.resize(size);
    for (uint64_t i = 0; i < size; i++) {
        in.read(reinterpret_cast<char *>(&byte), sizeof(byte));
        uint64_t operand = 0;
        if (takes_operand(static_cast<OPCODE>(byte))) {
            in.read(reinterpret_cast<char *>(&operand), sizeof(operand));
        }
        out.code[i] = {static_cast<OPCODE>(byte), {operand}};
    }
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    out.data.resize(size);
    for (uint64_t i = 0; i < size; i++) {
        in.read(reinterpret_cast<char *>(&byte), sizeof(byte));
        out.data[i] = byte;
    }
    return out;
}
inline void dump_bytecode(const program &ins, std::ofstream &out) {
    uint64_t size = ins.header.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (char i : ins.header) {
        out.write(&i, sizeof(i));
    }
    size = ins.code.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (auto &i : ins.code) {
        out.write(reinterpret_cast<const char *>(&i.op), sizeof(i.op));
        if (takes_operand(i.op)) {
            out.write(reinterpret_cast<const char *>(i.operands), sizeof(i.operands));
        }
    }
    size = ins.data.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (char i : ins.data) {
        out.write(&i, sizeof(i));
    }
}
} // namespace bvm
